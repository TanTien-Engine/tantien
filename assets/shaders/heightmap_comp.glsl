#define LOCAL_SIZE 32
layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;

layout(binding = 0, r16f) writeonly uniform image2D out_tex;

uniform sampler2D in_tex;

void main()
{
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    vec2 TexCoord = pos / float(imageSize(out_tex));
    __ASSIGN_CS_OUT__
    imageStore(out_tex, pos, vec4(cs_out));
}