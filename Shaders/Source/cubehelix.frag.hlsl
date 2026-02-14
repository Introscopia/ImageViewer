static const float     PI = 3.141592653589793238462643383279502884197169399375105820974;
static const float TWO_PI = 6.283185307179586476925286766559005768394338798750211641949;

cbuffer Uniforms : register(b0, space3) {
    float u_time : packoffset(c0);
}

float4 cubehelix(float t) {

    t = frac(t);
    
    // Cubehelix parameters
    float startHue = 0.0;   // degrees (purple start)
    float rotations = -1.0; // how many hue spins
    float saturation = 1.2;
    
    // Bell-shaped lightness
    float l = 0.3 + 0.4 * sin(t * PI);
    float angle = TWO_PI * (startHue / 360.0 + rotations * t);
    float a = saturation * l * (1.0 - l);
    
    // Calculate RGB components
    float r = l + a * (-0.14861 * cos(angle) + 1.78277 * sin(angle));
    float g = l + a * (-0.29227 * cos(angle) - 0.90649 * sin(angle));
    float b = l + a * (1.97294 * cos(angle));
    
    // Constrain and convert to 0-1 range for HLSL
    r = clamp(r, 0.0, 1.0);
    g = clamp(g, 0.0, 1.0);
    b = clamp(b, 0.0, 1.0);
    
    return float4(r, g, b, 1.0); // RGBA with full alpha
}

float4 main() : SV_Target {
    return cubehelix(u_time);
}