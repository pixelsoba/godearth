shader_type spatial;
render_mode cull_disabled, depth_draw_opaque;

uniform vec2  tile_origin = vec2(0.0, 0.0); // monde XY
uniform float tile_size   = 1.0;             // monde

void vertex() {
	vec2 uv = VERTEX.xz;
	vec2 world_xy = tile_origin + uv * tile_size;
	vec3 world_pos = vec3(world_xy.x, 0.0, world_xy.y);
	VERTEX = (VIEW_MATRIX * vec4(world_pos, 1.0)).xyz;
	NORMAL = vec3(0.0, 1.0, 0.0);
}

void fragment() {
	ALBEDO = vec3(0.4, 0.6, 0.4);
}
