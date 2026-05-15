// Vertex and fragment shader to draw a textured quad on screen.

cbuffer Uniforms
{
    float4x4 wvp_matrix;
    bool is_src_reverse_z;
    bool is_dst_reverse_z;
    Texture2D src_color_texture;
    Texture2D<float> src_depth_texture;
    SamplerState sampler;
}

struct VSIn
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct VSOut
{
    float2 uv : UV;
    float4 sv_position : SV_Position;
};

struct FSOut
{
    float4 color : SV_Target;
    float depth : SV_Depth;
};

// Vertex Shader
[shader("vertex")]
VSOut vs_main(VSIn input)
{
    VSOut output;
    output.uv = input.uv;
    output.sv_position = mul(float4(input.position, 1.0), wvp_matrix);

    return output;
}

// Fragment Shader
[shader("fragment")]
FSOut fs_main(float2 uv: UV)
{
    FSOut output;
    output.color = src_color_texture.Sample(sampler, uv);
    float depth = src_depth_texture.Sample(sampler, uv);
    
    // Convert depth value between reverse_z formats if needed
    if (is_src_reverse_z != is_dst_reverse_z) {
        depth = 1.0 - depth;
    }
    
    output.depth = depth;
    return output;
}
