#include <mesh/royma_terrain.h>
#include <material/royma_texture_2d.h>

namespace royma
{
	REGISTER_CLASS(Terrain);

	Terrain::~Terrain()
	{

	}

	strong<Shader> Terrain::getDefaultVertexShader()
	{
		return ShaderSource::getShader(L"static_transform");
	}

	/*
	strong<Shader> Terrain::getDefaultPixelShader()
	{
		Map<utf8, utf8> mapMacro;
		mapMacro["MATERIAL_INPUT"] = "terrain_material.mat";
		mapMacro["NORMAL_MAP"] = "";

		return ShaderSource::getShader(L"paint", mapMacro);
	}*/

	void Terrain::generateDrawableGeometries(bool bSmoothNormal)
	{
		Mesh::generateDrawableGeometries(bSmoothNormal);

		auto& geometries = getGeometries();
		if (geometries.count() == 0)
		{
			return;
			throw(EInvalidData(formatString(L"模型几何体缺失!, file path =%?", getFilePath())));
		}

		strong<Geometry> spGeom;
		strong<Shader> spShader;

		for (auto itGeom = geometries.begin(); itGeom != geometries.end(); itGeom++)
		{
			spGeom = *itGeom;
			spShader = spGeom->getVertexShader();
			if (spShader == nullptr)
			{
				spShader = getDefaultVertexShader();
			}

			Buffer<SVertexLayout> vertexLayout = spShader->getVertexLayout();

			spGeom->generateVertexBuffer(vertexLayout);
		}
	}

	void Terrain::load(const string& strFilePath)
	{
		//Class::load(strFilePath);

		//setFilePath(strFilePath);
		//loadTextures();
	}

	//void Terrain::generate(strong<Level> world)
	//{
	//	getGeometries().clear();

	//	strong<ImageBuffer> heightMap = world->getHeightMap();
	//	uint nWidth = (heightMap->width() - 1) / 64 + 1;
	//	uint nHeight = (heightMap->height() -1) / 64 + 1;

	//	sint nQuadCount = (nWidth - 1) * (nHeight - 1);
	//	Buffer<uint> bufIndices(nQuadCount * 4);
	//	sint nIndex00, nIndex01, nIndex10, nIndex11;
	//	uint nIndexInQuadBuffer = 0;

	//	for (uint nRow = 0; nRow < nHeight - 1; nRow++)
	//	{
	//		for (uint nCol = 0; nCol < nWidth - 1; nCol++)
	//		{
	//			nIndex00 = nRow * nWidth + nCol;
	//			nIndex01 = nRow * nWidth + (nCol + 1);
	//			nIndex10 = (nRow + 1)*nWidth + nCol;
	//			nIndex11 = (nRow + 1)*nWidth + (nCol + 1);

	//			bufIndices[nIndexInQuadBuffer++] = nIndex00;
	//			bufIndices[nIndexInQuadBuffer++] = nIndex01;
	//			bufIndices[nIndexInQuadBuffer++] = nIndex11;
	//			bufIndices[nIndexInQuadBuffer++] = nIndex10;
	//		}
	//	}

	//	slong nVerticesCount = nWidth * nHeight;

	//	Buffer<float> bufPosition(nVerticesCount * 3);
	//	Buffer<float> bufTexCoord(nVerticesCount * 2);
	//	//Buffer<float> bufAlphaCoord(nVerticesCount * 2);
	//	float3 vNormal;
	//	uint nIndex;

	//	matrix mScaling = world->getTransform();

	//	for (uint nRow = 0; nRow < nHeight; nRow++)
	//	{
	//		for (uint nCol = 0; nCol < nWidth; nCol++)
	//		{
	//			nIndex = nRow * nWidth + nCol;

	//			bufPosition[nIndex * 3 + 0] = nCol * mScaling._1_1 * 64.0f;
	//			bufPosition[nIndex * 3 + 1] = nRow * mScaling._2_2 * 64.0f;
	//			bufPosition[nIndex * 3 + 2] = heightMap->getPixel(nCol * 64, nRow * 64).r;

	//			bufTexCoord[nIndex * 2 + 0] = float(nCol) / (nWidth - 1);
	//			bufTexCoord[nIndex * 2 + 1] = float(nRow) / (nHeight - 1);

	//			/*bufAlphaCoord[nIndex * 2 + 0] = float(nCol) / (nWidth - 1);
	//			bufAlphaCoord[nIndex * 2 + 1] = float(nHeight - 1 - nRow) / (nHeight - 1);*/
	//		}
	//	}

	//	strong<Geometry> spGeometry = ResourceManager::create<Geometry>();
	//	spGeometry->setSource(SVertexLayout::giveSourceId(VS_SEMANTIC::POSITION), bufPosition);
	//	spGeometry->setSource(SVertexLayout::giveSourceId(VS_SEMANTIC::TEXCOORD), bufTexCoord);
	//	spGeometry->setIndexBuffer(bufIndices);
	//	spGeometry->setPrimitivesCount(nQuadCount);
	//	spGeometry->setIndexedDrawing(true);
	//	spGeometry->setPrimitiveType(PRIMITIVE_TYPE::QUAD_PATCH);

	//	getGeometries().insert(spGeometry);
	//	generateDrawableGeometries(true);
	//}

	void Terrain::generate()
	{
		getGeometries().clear();

		uint nWidth = HeightMap->width();
		uint nHeight = HeightMap->height();

		sint nQuadCount = (nWidth - 1) * (nHeight - 1);
		Buffer<uint> bufIndices(nQuadCount * 6);
		sint nIndex00, nIndex01, nIndex10, nIndex11;
		uint nIndexInQuadBuffer = 0;

		for (uint nRow = 0; nRow < nHeight - 1; nRow++)
		{
			for (uint nCol = 0; nCol < nWidth - 1; nCol++)
			{
				nIndex00 = nRow * nWidth + nCol;
				nIndex01 = nRow * nWidth + (nCol + 1);
				nIndex10 = (nRow + 1) * nWidth + nCol;
				nIndex11 = (nRow + 1) * nWidth + (nCol + 1);

				bufIndices[nIndexInQuadBuffer++] = nIndex00;
				bufIndices[nIndexInQuadBuffer++] = nIndex01;
				bufIndices[nIndexInQuadBuffer++] = nIndex10;
				bufIndices[nIndexInQuadBuffer++] = nIndex10;
				bufIndices[nIndexInQuadBuffer++] = nIndex01;
				bufIndices[nIndexInQuadBuffer++] = nIndex11;
			}
		}

		slong nVerticesCount = nWidth * nHeight;
		Buffer<float> bufPosition(nVerticesCount * 3);
		Buffer<float> bufTexCoord(nVerticesCount * 2);
		float3 vNormal;
		uint nIndex;

		for (uint nRow = 0; nRow < nHeight; ++nRow)
		{
			for (uint nCol = 0; nCol < nWidth; ++nCol)
			{
				nIndex = nRow * nWidth + nCol;

				bufPosition[nIndex * 3 + 0] = castval<float>(nCol);
				bufPosition[nIndex * 3 + 1] = castval<float>(nRow);
				bufPosition[nIndex * 3 + 2] = HeightMap->getPixel(nCol, nRow).r;

				bufTexCoord[nIndex * 2 + 0] = castval<float>(nCol) / chunkSize;
				bufTexCoord[nIndex * 2 + 1] = castval<float>(nRow) / chunkSize;
			}
		}

		strong<Geometry> spGeometry = ResourceManager::create<Geometry>();
		spGeometry->setSource(SVertexLayout::giveSourceId(VS_SEMANTIC::POSITION), bufPosition);
		spGeometry->setSource(SVertexLayout::giveSourceId(VS_SEMANTIC::TEXCOORD), bufTexCoord);
		spGeometry->setIndexBuffer(bufIndices);
		spGeometry->setIndexedDrawing(true);
		spGeometry->setPrimitivesCount(nQuadCount * 2);
		getGeometries().insert(spGeometry);
		generateDrawableGeometries(true);
	}

	void Terrain::generateCenterChunk(const uint3& square)
	{
		uint nStep = square.z / chunkSize;
		for (uint nCol = square.x + nStep; nCol < square.x + square.z - nStep; nCol += nStep)
		{
			for (uint nRow = square.y + nStep; nRow <= square.y + square.z - nStep; nRow += nStep)
			{
				dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
				dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol + nStep;
			}
			dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;
			dynamicPrimitiveCount += (chunkSize - 2) * 2;
		}
	}

	void Terrain::generateEastEdge(const uint3& square)
	{
		uint nStep = square.z / chunkSize;
		uint nCol = square.x + square.z;

		uint nRow = square.y;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
		for (nRow = square.y + nStep; nRow <= square.y + square.z - nStep; nRow += nStep)
		{
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol - nStep;
		}
		nRow = square.y + square.z;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;

		dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;
		dynamicPrimitiveCount += chunkSize * 2 - 2;
	}

	void Terrain::generateWestEdge(const uint3& square)
	{
		uint nStep = square.z / chunkSize;
		uint nCol = square.x;

		uint nRow = square.y;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
		for (nRow = square.y + nStep; nRow <= square.y + square.z - nStep; nRow += nStep)
		{
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol + nStep;
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
		}
		nRow = square.y + square.z;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;

		dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;
		dynamicPrimitiveCount += chunkSize * 2 - 2;
	}

	void Terrain::generateSouthEdge(const uint3& square)
	{
		uint nStep = square.z / chunkSize;
		uint nRow = square.y;

		uint nCol = square.x;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
		for (nCol = square.x + nStep; nCol <= square.x + square.z - nStep; nCol += nStep)
		{
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
			dynamicIndexBuffer[dynamicIndex++] = (nRow + nStep) * dynamicWidth + nCol;
		}
		nCol = square.x + square.z;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;

		dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;
		dynamicPrimitiveCount += chunkSize * 2 - 2;
	}

	void Terrain::generateNorthEdge(const uint3& square)
	{
		uint nStep = square.z / chunkSize;
		uint nRow = square.y + square.z;

		uint nCol = square.x;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
		for (nCol = square.x + nStep; nCol <= square.x + square.z - nStep; nCol += nStep)
		{
			dynamicIndexBuffer[dynamicIndex++] = (nRow - nStep) * dynamicWidth + nCol;
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
		}
		nCol = square.x + square.z;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;

		dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;
		dynamicPrimitiveCount += chunkSize * 2 - 2;
	}

	void Terrain::generateEastEdgeMorph(const uint3& square)
	{
		uint nHalfStep = square.z / chunkSize;
		uint nStep = 2 * nHalfStep;
		uint nCol = square.x + square.z - nHalfStep;

		uint nRow = square.y + nStep;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = (nRow - nHalfStep) * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol + nHalfStep;
		dynamicIndexBuffer[dynamicIndex++] = (nRow - nStep) * dynamicWidth + nCol + nHalfStep;
		dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;

		for (nRow = square.y + nStep; nRow < square.y + square.z - nStep; nRow += nStep)
		{
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol + nHalfStep;
			dynamicIndexBuffer[dynamicIndex++] = (nRow + nHalfStep) * dynamicWidth + nCol;
			dynamicIndexBuffer[dynamicIndex++] = (nRow + nStep) * dynamicWidth + nCol + nHalfStep;
			dynamicIndexBuffer[dynamicIndex++] = (nRow + nStep) * dynamicWidth + nCol;
			dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;
		}

		nRow = square.y + square.z - nStep;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol + nHalfStep;
		dynamicIndexBuffer[dynamicIndex++] = (nRow + nHalfStep) * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = (nRow + nStep) * dynamicWidth + nCol + nHalfStep;
		dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;

		dynamicPrimitiveCount += chunkSize / 2 * 3 - 2;
	}

	void Terrain::generateWestEdgeMorph(const uint3& square)
	{
		uint nHalfStep = square.z / chunkSize;
		uint nStep = 2 * nHalfStep;
		uint nCol = square.x + nHalfStep;

		uint nRow = square.y + nStep;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol - nHalfStep;
		dynamicIndexBuffer[dynamicIndex++] = (nRow - nHalfStep) * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = (nRow - nStep) * dynamicWidth + nCol - nHalfStep;
		dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;

		for (nRow = square.y + 2 * nStep; nRow < square.y + square.z; nRow += nStep)
		{
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol - nHalfStep;
			dynamicIndexBuffer[dynamicIndex++] = (nRow - nHalfStep) * dynamicWidth + nCol;
			dynamicIndexBuffer[dynamicIndex++] = (nRow - nStep) * dynamicWidth + nCol - nHalfStep;
			dynamicIndexBuffer[dynamicIndex++] = (nRow - nStep) * dynamicWidth + nCol;
			dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;
		}

		nRow = square.y + square.z - nStep;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = (nRow + nHalfStep) * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol - nHalfStep;
		dynamicIndexBuffer[dynamicIndex++] = (nRow + nStep) * dynamicWidth + nCol - nHalfStep;
		dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;

		dynamicPrimitiveCount += chunkSize / 2 * 3 - 2;
	}

	void Terrain::generateSouthEdgeMorph(const uint3& square)
	{
		uint nHalfStep = square.z / chunkSize;
		uint nStep = 2 * nHalfStep;
		uint nRow = square.y + nHalfStep;

		uint nCol = square.x + nStep;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol - nHalfStep;
		dynamicIndexBuffer[dynamicIndex++] = (nRow - nHalfStep) * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = (nRow - nHalfStep) * dynamicWidth + nCol - nStep;
		dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;

		for (nCol = square.x + nStep; nCol < square.x + square.z - nStep; nCol += nStep)
		{
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
			dynamicIndexBuffer[dynamicIndex++] = (nRow - nHalfStep) * dynamicWidth + nCol;
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol + nHalfStep;
			dynamicIndexBuffer[dynamicIndex++] = (nRow - nHalfStep) * dynamicWidth + nCol + nStep;
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol + nStep;
			dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;
		}

		nCol = square.x + square.z - nStep;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = (nRow - nHalfStep) * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol + nHalfStep;
		dynamicIndexBuffer[dynamicIndex++] = (nRow - nHalfStep) * dynamicWidth + nCol + nStep;
		dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;

		dynamicPrimitiveCount += chunkSize / 2 * 3 - 2;
	}

	void Terrain::generateNorthEdgeMorph(const uint3& square)
	{
		uint nHalfStep = square.z / chunkSize;
		uint nStep = 2 * nHalfStep;
		uint nRow = square.y + square.z - nHalfStep;

		uint nCol = square.x + nStep;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = (nRow + nHalfStep) * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol - nHalfStep;
		dynamicIndexBuffer[dynamicIndex++] = (nRow + nHalfStep) * dynamicWidth + nCol - nStep;
		dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;

		for (nCol = square.x + 2 * nStep; nCol < square.x + square.z; nCol += nStep)
		{
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
			dynamicIndexBuffer[dynamicIndex++] = (nRow + nHalfStep) * dynamicWidth + nCol;
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol - nHalfStep;
			dynamicIndexBuffer[dynamicIndex++] = (nRow + nHalfStep) * dynamicWidth + nCol - nStep;
			dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol - nStep;
			dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;
		}

		nCol = square.x + square.z - nStep;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = nRow * dynamicWidth + nCol + nHalfStep;
		dynamicIndexBuffer[dynamicIndex++] = (nRow + nHalfStep) * dynamicWidth + nCol;
		dynamicIndexBuffer[dynamicIndex++] = (nRow + nHalfStep) * dynamicWidth + nCol + nStep;
		dynamicIndexBuffer[dynamicIndex++] = 0xffffFFFF;

		dynamicPrimitiveCount += chunkSize / 2 * 3 - 2;
	}

	void Terrain::update(strong<Camera> spCamera)
	{
		SQuadTree quadTree;
		auto pCamera = spCamera.get();
		quadTree.generate({ 0, 0, castval<uint>(HeightMap->width() - 1) }, [this, pCamera](SQuadTree::Branch* pBranch)->bool
		{
			uint2 indices[4] = { { pBranch->square.x, pBranch->square.y },
				{ pBranch->square.x + pBranch->square.z, pBranch->square.y },
				{ pBranch->square.x, pBranch->square.y + pBranch->square.z },
				{ pBranch->square.x + pBranch->square.z, pBranch->square.y + pBranch->square.z } };

			float3 vertices[4];
			for (size_t i = 0; i < 4; ++i)
			{
				vertices[i] = float3(indices[i].x, indices[i].y, HeightMap->getPixel(indices[i].x, indices[i].y).r);
			}
			vector center = 0.25f * (vertices[0] + vertices[1] + vertices[2] + vertices[3]);

			auto error = [](float e, Camera* pCamera, vector center)->float
			{
				float d = distance(pCamera->getPosition(), center);
				float s = e * pCamera->getHeight() / (2.0f * d * tan(0.5f * pCamera->getFov()));
				return s;
			};

			if (pBranch->square.z <= chunkSize || error(pBranch->square.z, pCamera, center) < 512.0f)
			{
				return false;
			}
			else
			{
				return true;
			}
		});

		assert(chunkSize > 3);
		dynamicIndex = 0;
		dynamicPrimitiveCount = 0;
		dynamicWidth = HeightMap->width();
		slong nIndicesCount = quadTree.count * ((chunkSize - 2) * ((chunkSize - 2 + 1) * 2 + 1) + 4 * ((chunkSize - 4) / 2 * 6 + 10));
		dynamicIndexBuffer.realloc(nIndicesCount);
		memset(dynamicIndexBuffer.data(), 0xff, dynamicIndexBuffer.size());
		
		quadTree.traversal([this](SQuadTree::Branch* pBranch)
		{
			generateCenterChunk(pBranch->square);
			if (pBranch->eastBranch)
			{
				generateEastEdge(pBranch->square);
			}
			else
			{
				generateEastEdgeMorph(pBranch->square);
			}
			if (pBranch->westBranch)
			{
				generateWestEdge(pBranch->square);
			}
			else
			{
				generateWestEdgeMorph(pBranch->square);
			}
			if (pBranch->southBranch)
			{
				generateSouthEdge(pBranch->square);
			}
			else
			{
				generateSouthEdgeMorph(pBranch->square);
			}
			if (pBranch->northBranch)
			{
				generateNorthEdge(pBranch->square);
			}
			else
			{
				generateNorthEdgeMorph(pBranch->square);
			}
		});

		auto spGeometry = getFirstGeometry();
		spGeometry->setPrimitivesCount(dynamicPrimitiveCount);
		spGeometry->setIndexBuffer(dynamicIndexBuffer);
		spGeometry->getDrawableMesh()->setIndexBuffer(dynamicIndexBuffer);
	}

	void Terrain::resetHeightMap(uint nSize, uint nChunkSize)
	{
		assert(nSize);
		assert(nSize % nChunkSize == 0);
		HeightMap = ResourceManager::create<ImageBuffer>(nSize + 1, nSize + 1, COLOR_FORMAT::FLOAT);
		HeightMap->zeroMemory();
		chunkSize = nChunkSize;
	}

	float Terrain::getTerrainHeight(slong nCol, slong nRow)
	{
		if (nCol < 0 || nRow < 0 || nCol >= castval<slong>(HeightMap->width()) || nRow >= castval<slong>(HeightMap->height()))
		{
			return 0.0f;
		}

		return HeightMap->getPixel(nCol, nRow).r;
	}

	float Terrain::getTerrainHeight(float x, float y)
	{
		slong nRow = slong(y / Transform._2_2);
		slong nCol = slong(x / Transform._1_1);

		if (nCol < 0 || nRow < 0 || nCol >= castval<int>(HeightMap->width()) || nRow >= castval<int>(HeightMap->height()))
		{
			return 0.0f;
		}

		vector scale = xyz(Transform._1_1, Transform._2_2, 1.0f);

		matrix tile = getTile(nCol, nRow);
		Triangle tri;
		vector dstPoint;

		float2 uv(x - nCol * Transform._1_1, y - nRow * Transform._2_2);

		if (isZero(uv.x))
		{
			dstPoint = lerp(tile.r[0], tile.r[3], uv.y);
		}
		else
		{
			if (less(uv.y / uv.x, 1.0f))
			{
				tri = Triangle(tile.r[1] * scale, tile.r[2] * scale, tile.r[0] * scale);
				dstPoint = tri.interpolation(float2(1.0f - uv.x, uv.y));
			}
			else
			{
				tri = Triangle(float3(tile.r[3]) * scale, float3(tile.r[0]) * scale, float3(tile.r[2]) * scale);
				dstPoint = tri.interpolation(float2(uv.x, 1.0f - uv.y));
			}
		}

		return getz(dstPoint);
	}

	vector Terrain::getTerrainNormal(slong nCol, slong nRow)
	{
		slong x0 = nCol;
		slong y0 = nRow;

		slong x1 = nCol + 1;
		slong y1 = nRow;

		slong x2 = nCol;
		slong y2 = nRow - 1;

		slong x3 = nCol - 1;
		slong y3 = nRow;

		slong x4 = nCol;
		slong y4 = nRow + 1;

		vector v0 = xyz((float)x0, getTerrainHeight(x0, y0), (float)y0);
		vector v1 = xyz((float)x1, getTerrainHeight(x1, y1), (float)y1);
		vector v2 = xyz((float)x2, getTerrainHeight(x2, y2), (float)y2);
		vector v3 = xyz((float)x3, getTerrainHeight(x3, y3), (float)y3);
		vector v4 = xyz((float)x4, getTerrainHeight(x4, y4), (float)y4);

		vector n1 = normal(v0, v1, v2);
		vector n2 = normal(v0, v2, v3);
		vector n3 = normal(v0, v3, v4);
		vector n4 = normal(v0, v4, v1);

		return normalize3(n1 + n2 + n3 + n4);
	}

	void Terrain::generateRandomHeightMap(float fMinHeight, float fMaxHeight, float fAccuracy)
	{
		float nDeltaHeight = fMaxHeight - fMinHeight;
		if (nDeltaHeight <= 0)
		{
			throw(EInvalidData(L"无效的参数!"));
		}

		srand(0);

		for (uint nRow = 0; nRow < HeightMap->height(); nRow++)
		{
			for (uint nCol = 0; nCol < HeightMap->width(); nCol++)
			{
				HeightMap->setPixel(nCol, nRow, LinearColor(fMinHeight + Float::random() * nDeltaHeight, 0.0f, 0.0f, 0.0f));
			}
		}
	}

	void Terrain::smooth(int nRefRange)
	{
		strong<ImageBuffer> smoothedHeightMap = ResourceManager::create<ImageBuffer>(HeightMap->width(), HeightMap->height(), HeightMap->colorFormat());

		//Buffer<size_t> globalWorkItems(2);
		//globalWorkItems[0] = smoothedHeightMap.height();
		//globalWorkItems[1] = smoothedHeightMap.width();

		//Buffer<size_t> localWorkItems(2);
		//localWorkItems[0] = 1;
		//localWorkItems[1] = 1;

		//GpGpu::global()->executeKernel(L"Terrain", L"smooth", globalWorkItems, localWorkItems, KernelArgImage((const strong<ImageBuffer>&)HeightMap), nRefRange, KernelArgImage(smoothedHeightMap, true));

		for (int nRow = 0; nRow < castval<int>(HeightMap->height()); ++nRow)
		{
			for (int nCol = 0; nCol < castval<int>(HeightMap->width()); ++nCol)
			{
				float fTotalHeight = 0.0f;
				for (int offsetY = -nRefRange; offsetY <= nRefRange; offsetY++)
				{
					for (int offsetX = -nRefRange; offsetX <= nRefRange; offsetX++)
					{
						fTotalHeight += HeightMap->getPixel(nCol + offsetX, nRow + offsetY).r;
					}
				}
				float fAverageHeight = fTotalHeight / (float)((nRefRange * 2 + 1) * (nRefRange * 2 + 1));
				smoothedHeightMap->setPixel(nCol, nRow, LinearColor(fAverageHeight, 0.0f, 0.0f));
			}
		}

		HeightMap = smoothedHeightMap;
	}
}