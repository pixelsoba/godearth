extends AbstractTessellator
class_name TetrahedronTessellator


static func create_mesh(number_of_subdivisions: int, vertex_attributes: TessellatorAttributes = TessellatorAttributes.ALL):
	if number_of_subdivisions < 0:
		push_error("number_of_subdivisions doit être >= 0")
		return null
	
	var st = SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	
	var vertices = []
	var normals = []
	var uvs = []
	var indices = []
	
	var negative_root_two_over_three = -sqrt(2.0) / 3.0
	var negative_one_third = -1.0 / 3.0
	var root_six_over_three = sqrt(6.0) / 3.0
	
	var p0 = Vector3(0, 0, 1)
	var p1 = Vector3(0, (2.0 * sqrt(2.0)) / 3.0, negative_one_third)
	var p2 = Vector3(-root_six_over_three, negative_root_two_over_three, negative_one_third)
	var p3 = Vector3(root_six_over_three, negative_root_two_over_three, negative_one_third)
	
	vertices.append(p0)
	vertices.append(p1)
	vertices.append(p2)
	vertices.append(p3)
	
	if vertex_attributes & TessellatorAttributes.NORMAL:
		normals.append(p0.normalized())
		normals.append(p1.normalized())
		normals.append(p2.normalized())
		normals.append(p3.normalized())
	
	if vertex_attributes & TessellatorAttributes.TEXTURE_COORDINATE:
		uvs.append(compute_texture_coordinate(p0))
		uvs.append(compute_texture_coordinate(p1))
		uvs.append(compute_texture_coordinate(p2))
		uvs.append(compute_texture_coordinate(p3))
	
	subdivide(vertices, normals, uvs, indices, 0, 2, 1, number_of_subdivisions, vertex_attributes)
	subdivide(vertices, normals, uvs, indices, 0, 3, 2, number_of_subdivisions, vertex_attributes)
	subdivide(vertices, normals, uvs, indices, 0, 1, 3, number_of_subdivisions, vertex_attributes)
	subdivide(vertices, normals, uvs, indices, 1, 2, 3, number_of_subdivisions, vertex_attributes)
	
	for i in range(vertices.size()):
		if vertex_attributes & TessellatorAttributes.NORMAL:
			st.set_normal(normals[i])
		if vertex_attributes & TessellatorAttributes.TEXTURE_COORDINATE:
			st.set_uv(uvs[i])
		st.add_vertex(vertices[i])
	
	for index in indices:
		st.add_index(index)
	
	st.generate_normals()
	return st.commit()


static func subdivide(vertices, normals, uvs, indices, i0, i1, i2, level, vertex_attributes):
	if level > 0:
		var p01 = ((vertices[i0] + vertices[i1]) / 2.0).normalized()
		var p12 = ((vertices[i1] + vertices[i2]) / 2.0).normalized()
		var p20 = ((vertices[i2] + vertices[i0]) / 2.0).normalized()
		
		var i01 = vertices.size()
		vertices.append(p01)
		var i12 = vertices.size()
		vertices.append(p12)
		var i20 = vertices.size()
		vertices.append(p20)
		
		if vertex_attributes & TessellatorAttributes.NORMAL:
			normals.append(p01.normalized())
			normals.append(p12.normalized())
			normals.append(p20.normalized())
		
		if vertex_attributes & TessellatorAttributes.TEXTURE_COORDINATE:
			uvs.append(compute_texture_coordinate(p01))
			uvs.append(compute_texture_coordinate(p12))
			uvs.append(compute_texture_coordinate(p20))
		
		level -= 1
		subdivide(vertices, normals, uvs, indices, i0, i01, i20, level, vertex_attributes)
		subdivide(vertices, normals, uvs, indices, i01, i1, i12, level, vertex_attributes)
		subdivide(vertices, normals, uvs, indices, i01, i12, i20, level, vertex_attributes)
		subdivide(vertices, normals, uvs, indices, i20, i12, i2, level, vertex_attributes)
	else:
		indices.append(i0)
		indices.append(i1)
		indices.append(i2)

static func compute_texture_coordinate(position):
	var u = 0.5 - atan2(position.z, position.x) / (2.0 * PI)
	var v = 0.5 - asin(position.y) / PI
	return Vector2(u, v)
