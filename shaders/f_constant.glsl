#version 330

out vec4 pixelColor; //Zmienna wyjsciowa fragment shadera. Zapisuje sie do niej ostateczny kolor piksela

#define MAX_LIGHTS 6
#define MAX_MAP_CELLS 100

uniform vec4 color = vec4(1, 1, 1, 1);
uniform sampler2D textureMap;
uniform int useTexture = 0;
uniform int useLighting = 1;
uniform int useShadow = 1;
uniform float ambientStrength = 0.08;

uniform int lampCount = 0;
uniform vec3 lampPositions[MAX_LIGHTS];
uniform vec3 lampColors[MAX_LIGHTS];
uniform float lampRadii[MAX_LIGHTS];
uniform float lampIntensities[MAX_LIGHTS];

uniform int mapWidth = 10;
uniform int mapHeight = 10;
uniform int wallMap[MAX_MAP_CELLS];

in vec2 i_tc;
in vec3 i_worldPos;
in vec3 i_normal;

int tileCoord(float value) {
    return int(floor((value + 1.0) * 0.5));
}

bool isWallCell(int x, int z) {
    if (x < 0 || x >= mapWidth || z < 0 || z >= mapHeight) {
        return true;
    }

    int index = z * mapWidth + x;
    if (index < 0 || index >= MAX_MAP_CELLS) {
        return true;
    }

    return wallMap[index] == 1;
}

float lampVisibility(vec3 worldPos, vec3 lampPos) {
    if (useShadow == 0) {
        return 1.0;
    }

    int startX = tileCoord(worldPos.x);
    int startZ = tileCoord(worldPos.z);
    int endX = tileCoord(lampPos.x);
    int endZ = tileCoord(lampPos.z);

    if ((startX != endX || startZ != endZ) && isWallCell(endX, endZ)) {
        return 0.0;
    }

    vec2 fromPos = worldPos.xz;
    vec2 toPos = lampPos.xz;
    float distanceXZ = length(toPos - fromPos);
    int steps = int(clamp(ceil(distanceXZ * 8.0), 1.0, 64.0));

    for (int i = 1; i <= 64; ++i) {
        if (i > steps) {
            break;
        }

        float t = float(i) / float(steps);
        vec2 samplePos = mix(fromPos, toPos, t);
        int x = tileCoord(samplePos.x);
        int z = tileCoord(samplePos.y);

        bool startCell = x == startX && z == startZ;
        bool endCell = x == endX && z == endZ;
        if (!startCell && !endCell && isWallCell(x, z)) {
            return 0.0;
        }
    }

    return 1.0;
}

void main(void) {
    vec4 baseColor;
    if (useTexture == 1) {
        baseColor = texture(textureMap, i_tc);
    } else {
        baseColor = color;
    }

    if (useLighting == 0) {
        pixelColor = baseColor;
        return;
    }

    vec3 normal = normalize(i_normal);
    vec3 lampLight = vec3(0.0);

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (i >= lampCount) {
            break;
        }

        vec3 toLamp = lampPositions[i] - i_worldPos;
        float distanceToLamp = length(toLamp);
        float radius = max(lampRadii[i], 0.001);
        float attenuation = clamp(1.0 - distanceToLamp / radius, 0.0, 1.0);
        attenuation *= attenuation;

        vec3 lightDir = normalize(toLamp);
        float diffuse = max(dot(normal, lightDir), 0.0);
        float softDiffuse = max(diffuse, 0.18);
        float downwardCone = dot(normalize(i_worldPos - lampPositions[i]), vec3(0.0, -1.0, 0.0));
        float cone = smoothstep(0.05, 0.78, downwardCone);
        float visible = lampVisibility(i_worldPos, lampPositions[i]);

        lampLight += lampColors[i] * softDiffuse * attenuation * cone * lampIntensities[i] * visible;
    }

    vec3 litColor = baseColor.rgb * clamp(vec3(ambientStrength) + lampLight, vec3(0.0), vec3(1.55));
    pixelColor = vec4(litColor, baseColor.a);
}
