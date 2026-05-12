// Vertex and fragment shader to draw a box on screen.

cbuffer Uniforms
{
    float4x4 world_matrix;
    float4x4 view_matrix;
    float4x4 proj_matrix;
}

struct VSIn
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct VSOut
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

// Vertex Shader
[shader("vertex")]
VSOut vs_main(VSIn input)
{
    VSOut output;
    output.color = input.color;
    float4x4 mvp_matrix = mul(world_matrix, mul(view_matrix, proj_matrix));
    output.position = mul(float4(input.position, 1.0), mvp_matrix);

    return output;
}

// Fragment Shader
[shader("fragment")]
float4 fs_main(VSOut input) : SV_Target
{
    return input.color;
}
