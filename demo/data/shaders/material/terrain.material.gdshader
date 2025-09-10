shader_type spatial;
render_mode cull_disabled, depth_draw_opaque;

varying flat int ID;

void vertex() {
    vec2 uv = VERTEX.xz;
	
    vec2 tile_origin = INSTANCE_CUSTOM.xy;
    float tile_size  = INSTANCE_CUSTOM.z;
    vec2 world_xy = tile_origin + uv * tile_size;
    vec3 world_pos = vec3(world_xy.x, 0.0, world_xy.y);
	ID = INSTANCE_ID;
    VERTEX = world_pos;
    NORMAL = vec3(0.0, 1.0, 0.0);
}

void fragment() {
    float lod_norm = float(ID % 50) / 50.0;
    ALBEDO = mix(vec3(0.2,0.4,0.8), vec3(0.8,0.4,0.2), lod_norm);
}
