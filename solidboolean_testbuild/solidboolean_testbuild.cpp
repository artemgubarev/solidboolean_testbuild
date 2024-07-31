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
	std::vector<Vector3> firstVertices;
	std::vector<std::vector<size_t>> firstTriangles;

	std::vector<Vector3> secondVertices;
	std::vector<std::vector<size_t>> secondTriangles;

	loadObj("C:\\Users\\gubarev.av\\Desktop\\plotly\\obj3d\\mesh0.obj", firstVertices, firstTriangles);
	loadObj("C:\\Users\\gubarev.av\\Desktop\\plotly\\obj3d\\mesh1.obj", secondVertices, secondTriangles);

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
