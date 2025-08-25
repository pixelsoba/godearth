extends Object
class_name AbstractTessellator

enum TessellatorAttributes {
	POSITION = 1,
	NORMAL = 2,
	TEXTURE_COORDINATE = 4,
	ALL = POSITION | NORMAL | TEXTURE_COORDINATE
}

static func create_mesh(number_of_subdivisions: int, vertex_attributes: TessellatorAttributes = TessellatorAttributes.ALL):
	pass
