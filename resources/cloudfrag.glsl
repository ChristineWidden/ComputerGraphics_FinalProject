#version 330 core
out vec4 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;

in vec3 vert_col;

uniform vec3 campos;
uniform vec2 texoff;

uniform float dontrender;

uniform sampler2D tex;
uniform sampler2D tex2;

uniform float dn;
void main()
{

	vec2 newTex = vertex_tex;
	newTex /= 8;




	vec3 fragTan = vec3(0, 1, 0);
	vec3 fragBinorm = vec3(1, 0, 0);
	vec3 normal = vec3(0, 0, 1);

	vec3 texturecolor = texture(tex, newTex + texoff).rgb;
	vec3 texturenormal = texture(tex2, newTex + texoff).rgb;

	texturenormal = (texturenormal - vec3(0.5,0.5,0.5))*2.0;

	// Calculate the normal from the data in the bump map.
    vec3 bumpNormal = (texturenormal.x * fragTan) + (texturenormal.y * fragBinorm) + (texturenormal.z * normal);
	bumpNormal = normalize(bumpNormal);

	vec3 lp = vec3(100,100,100);
	vec3 ld = normalize(lp - vertex_pos);
	float light = dot(ld,bumpNormal);	
	light = clamp(light,0,1);

	vec3 camvec = normalize(campos - vertex_pos);
	vec3 h = normalize(camvec+ld);
	float spec = pow(dot(h,bumpNormal),5);
	spec = clamp(spec,0,1);
	
	color.rgb = texturecolor * (light * 0.5f + 0.75f) + 0.1f * vec3(1,1,1)*spec;
	

	color.a= (texture(tex, newTex + texoff).a);


	if (dontrender > 0.5) {

		color = vec4 (0, 0, 0, 1);

	}


	


}
