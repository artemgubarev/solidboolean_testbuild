#define CONVHULL_3D_ENABLE

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include "vector3.h"
#include "solidmesh.h"
#include "solidboolean.h"
#include "convhull_3d.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "section_remesher.h"

void ReadOFFFile(const std::string& filename, std::vector<double>& vertices, std::vector<int>& indices) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Error opening file: " << filename << std::endl;
		return;
	}

	std::string line;
	std::getline(file, line); // Пропустить строку "OFF"

	int numVertices, numFaces, unused;
	file >> numVertices >> numFaces >> unused;

	// Считать вершины
	vertices.resize(numVertices * 3);
	for (int i = 0; i < numVertices * 3; ++i) {
		file >> vertices[i];
	}

	// Считать индексы треугольников
	indices.resize(numFaces * 3);
	for (int i = 0; i < numFaces; ++i) {
		int numIndices;
		file >> numIndices; // Пропустить число индексов в строке (оно всегда равно 3 для треугольников)
		for (int j = 0; j < 3; ++j) {
			file >> indices[i * 3 + j];
			indices[i * 3 + j]++;
		}
	}

	file.close();
}

static void exportObject(const char* filename, const std::vector<Vector3>& vertices, const std::vector<std::vector<size_t>>& faces)
{
	FILE* fp = fopen(filename, "wb");
	for (const auto& it : vertices) {
		fprintf(fp, "v %f %f %f\n", it.x(), it.y(), it.z());
	}
	for (const auto& it : faces) {
		if (it.size() == 2) {
			fprintf(fp, "l");
			for (const auto& v : it)
				fprintf(fp, " %zu", v + 1);
			fprintf(fp, "\n");
			continue;
		}
		fprintf(fp, "f");
		for (const auto& v : it)
			fprintf(fp, " %zu", v + 1);
		fprintf(fp, "\n");
	}
	fclose(fp);
}

static bool loadObj(const std::string& filename,
	std::vector<Vector3>& outputVertices,
	std::vector<std::vector<size_t>>& outputTriangles)
{
	tinyobj::attrib_t attributes;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	bool loadSuccess = tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, filename.c_str());
	if (!warn.empty()) {
		std::cout << "WARN:" << warn.c_str() << std::endl;
	}
	if (!err.empty()) {
		std::cout << err.c_str() << std::endl;
	}
	if (!loadSuccess) {
		return false;
	}

	outputVertices.resize(attributes.vertices.size() / 3);
	for (size_t i = 0, j = 0; i < outputVertices.size(); ++i) {
		auto& dest = outputVertices[i];
		dest.setX(attributes.vertices[j++]);
		dest.setY(attributes.vertices[j++]);
		dest.setZ(attributes.vertices[j++]);
	}

	outputTriangles.clear();
	for (const auto& shape : shapes) {
		for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
			outputTriangles.push_back(std::vector<size_t> {
				(size_t)shape.mesh.indices[i + 0].vertex_index,
					(size_t)shape.mesh.indices[i + 1].vertex_index,
					(size_t)shape.mesh.indices[i + 2].vertex_index
			});
		}
	}

	return true;
}

int main()
{
	std::vector<Vector3> firstVertices = 
	{
		{334.037841796875, 0, 381.257720947266},
		{644.580139160156, 0, 1489.46728515625},
		{1399.62426757813, 0, 1318.9736328125},
		{334.037841796875, 450, 381.257720947266},
		{644.580139160156, 450 ,1489.46728515625},
		{1399.62426757813, 450, 1318.9736328125},
	};
	std::vector<std::vector<size_t>> firstTriangles =
	{
		{0 ,3, 2},
		{0 ,2, 1},
		{0, 4 ,3},
		{0, 1 ,4},
		{3, 5 ,2},
		{4, 5, 3},
		{2, 5, 1},
		{1, 5 ,4},

	};

	std::vector<Vector3> secondVertices =
	{
		{0,0,0},
		{850,0,0},
		{850,200,0},
		{0,200,0},
		{0,0,850},
		{850,0,850},
		{850,200,850},
		{0,200,850},
	};
	std::vector<std::vector<size_t>> secondTriangles =
	{
		{0,3,2},
		{0,2,1},
		{3,6,2},
		{2,6,1},
		{0,1,4},
		{6,5,1},
		{1,5,4},
		{0,7,3},
		{0,4,7},
		{3,7,6},
		{6,7,5},
		{5,7,4},
	};


	auto t1 = std::chrono::high_resolution_clock::now();

	SolidMesh firstMesh;
	firstMesh.setVertices(&firstVertices);
	firstMesh.setTriangles(&firstTriangles);
	firstMesh.prepare();

	SolidMesh secondMesh;
	secondMesh.setVertices(&secondVertices);
	secondMesh.setTriangles(&secondTriangles);
	secondMesh.prepare();

	SolidBoolean solidBoolean(&firstMesh, &secondMesh);
	solidBoolean.combine();

	std::vector<std::vector<size_t>> resultTriangles;
	solidBoolean.fetchIntersect(resultTriangles);

	return 0;
}
