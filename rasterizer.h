#pragma once
#include <windows.h>
#include <vector>

class Rasterizer {
	private:
		int attributesPerVertex;
		int varyingsPerVertex;
		void renderTriangle(float v1xt, float v1yt, float v2xt, float v2yt, float v3xt, float v3yt, std::vector<float> &varyings, std::vector<float> &varyings1, std::vector<float> &varyings2, std::vector<float> &varyings3, int width, int height, float iv1w, float iv2w, float iv3w, float v1zp, float v2zp, float v3zp, COLORREF* (&out), float* ((*fragment)(std::vector<float> &varyings, int x, int y)), std::vector<float> &depth);
	public:
        std::vector<float> uniforms;
        std::vector<float> vertices;
        std::vector<int> indices;
		Rasterizer(int attributesPerVertex, int varyingsPerVertex);
		void renderTo(COLORREF* (&out), int width, int height, float* ((*vertex)(std::vector<float> &attributes, std::vector<float> &varyings)), float* ((*fragment)(std::vector<float> &varyings, int x, int y)), std::vector<float> &depth);
};
