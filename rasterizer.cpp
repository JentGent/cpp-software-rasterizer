#include "rasterizer.h"
#include <windows.h>
#include <vector>
#include <iostream>

#define CLIP 0.005
#define INV_CLIP 200

Rasterizer::Rasterizer(int attributesPerVertex, int varyingsPerVertex) {
	this->attributesPerVertex = attributesPerVertex;
	this->varyingsPerVertex = varyingsPerVertex;
}

void Rasterizer::renderTriangle(float v1xt, float v1yt, float v2xt, float v2yt, float v3xt, float v3yt, std::vector<float> &varyings, std::vector<float> &varyings1, std::vector<float> &varyings2, std::vector<float> &varyings3, int width, int height, float iv1w, float iv2w, float iv3w, float v1zp, float v2zp, float v3zp, COLORREF* (&out), float* ((*fragment)(std::vector<float> &varyings, int x, int y)), std::vector<float> &depth) {
    int j = (int)(v1xt < v2xt ? (v1xt < v3xt ? v1xt : v3xt) : (v2xt < v3xt ? v2xt : v3xt)) + 1;
    if(j < 0) { j = 0; }
    float xMin = v1xt, yMin = v1yt, xMiddle = 0, yMiddle = 0, xMax = v3xt, yMax = v3yt;
    int idMin = 0, idMiddle = 1, idMax = 2;
    if (v2xt < xMin) { xMin = v2xt; yMin = v2yt; idMin = 1; }
    if (v3xt < xMin) { xMin = v3xt; yMin = v3yt; idMin = 2; }
    if (v1xt > xMax) { xMax = v1xt; yMax = v1yt; idMax = 0; }
    if (v2xt > xMax) { xMax = v2xt; yMax = v2yt; idMax = 1; }
    idMiddle = 3 - idMin - idMax;
    if(!idMiddle) { xMiddle = v1xt; yMiddle = v1yt; }
    else if(idMiddle == 1) { xMiddle = v2xt; yMiddle = v2yt; }
    else if(idMiddle == 2) { xMiddle = v3xt; yMiddle = v3yt; }
    float inv = 1.f / ((v2xt - v1xt) * (v3yt - v1yt) - (v3xt - v1xt) * (v2yt - v1yt));
    float divMax = 1.f / (xMax - xMin), divLeft = 1.f / (xMiddle - xMin), divRight = 1.f / (xMax - xMiddle);
    for(j = j; j < (v1xt > v2xt ? (v1xt > v3xt ? v1xt : v3xt) : (v2xt > v3xt ? v2xt : v3xt)); j += 1) {
        if(j >= width) { break; }
        int y1 = (int)(j < xMiddle ? yMin + (yMiddle - yMin) * (j - xMin) * divLeft : yMiddle + (yMax - yMiddle) * (j - xMiddle) * divRight), y2 = (int)(yMin + (yMax - yMin) * (j - xMin) * divMax);
        if(y1 < 0) { y1 = 0; }
        if(y2 < 0) { y2 = 0; }
        if(y1 > y2) {
            int t = y2;
            y2 = y1;
            y1 = t;
        }
        for(int k = y1 + 1; k < y2 + 1; k += 1) {
            if(k >= height) { break; }
            float w2 = ((j - v1xt) * (v3yt - v1yt) - (v3xt - v1xt) * (k - v1yt)) * inv;
            float w3 = ((v2xt - v1xt) * (k - v1yt) - (j - v1xt) * (v2yt - v1yt)) * inv;
            float w1 = 1.f - w3 - w2;
            if(w1 < 0) { w1 = -w1; w2 = -w2; w3 = -w3; }
            float iw = 1.f / (iv1w * w1 + iv2w * w2 + iv3w * w3);
            float z = (v1zp * w1 + v2zp * w2 + v3zp * w3) * iw;
            float de = depth[j + k * width];
            if(!de || z <= de) {
                for(int l = 0; l < this->varyingsPerVertex; l += 1) {
                    varyings[l] = (varyings1[l] * iv1w * w1 + varyings2[l] * iv2w * w2 + varyings3[l] * iv3w * w3) * iw;
                }
                float* c = fragment(varyings, j, k);
                if(!c) { continue; }
                depth[j + k * width] = z;
                out[j + k * width] = (((int)(*(c)) & 0xff) << 16) + (((int)(*(c + 1)) & 0xff) << 8) + (((int)(*(c + 2)) | 0) & 0xff);
            }
        }
    }
}

void Rasterizer::renderTo(COLORREF* (&out), int width, int height, float* ((*vertex)(std::vector<float> &attributes, std::vector<float> &varyings)), float* ((*fragment)(std::vector<float> &varyings, int x, int y)), std::vector<float> &depth) {
    std::vector<float> vertices;
    int a = 0;
    std::vector<float> attributes(this->attributesPerVertex);
    std::vector<float> varyings(this->varyingsPerVertex);
    std::vector<float> varyings1(this->varyingsPerVertex);
    std::vector<float> varyings2(this->varyingsPerVertex);
    std::vector<float> varyings3(this->varyingsPerVertex);
    for(int i = 0; i < this->vertices.size(); i += 1) {
        attributes[a] = this->vertices[i];
        a += 1;
        if(a == this->attributesPerVertex) {
            a = 0;
            float* pv = vertex(attributes, varyings);
            if(pv) {
                vertices.push_back(*(pv));
                vertices.push_back(*(pv + 1));
                vertices.push_back(*(pv + 2));
                vertices.push_back(*(pv + 3));
                for(int j = 0; j < this->varyingsPerVertex; j += 1) {
                    vertices.push_back(varyings[j]);
                }
            }
            else {
                vertices.push_back(0);
                vertices.push_back(0);
                vertices.push_back(0);
                vertices.push_back(0);
                for(int j = 0; j < this->varyingsPerVertex; j += 1) {
                    vertices.push_back(0);
                }
            }
        }
    }
    for(int i = 0; i < this->indices.size(); i += 3) {
        int v1 = this->indices[i] * (4 + this->varyingsPerVertex), v2 = this->indices[i + 1] * (4 + this->varyingsPerVertex), v3 = this->indices[i + 2] * (4 + this->varyingsPerVertex);
        float v1x = vertices[v1], v1y = vertices[v1 + 1], v1z = vertices[v1 + 2], v1w = vertices[v1 + 3],
            v2x = vertices[v2], v2y = vertices[v2 + 1], v2z = vertices[v2 + 2], v2w = vertices[v2 + 3],
            v3x = vertices[v3], v3y = vertices[v3 + 1], v3z = vertices[v3 + 2], v3w = vertices[v3 + 3];
        if(v1w > CLIP || v2w > CLIP || v3w > CLIP) {
            float iv1w, iv2w, iv3w, v1xp, v1yp, v1zp, v2xp, v2yp, v2zp, v3xp, v3yp, v3zp, v1xt, v1yt, v2xt, v2yt, v3xt, v3yt;
            bool clipIntoTwo = false;
            if(v1w > CLIP && v2w > CLIP && v3w > CLIP) {
                iv1w = 1.f / v1w; iv2w = 1.f / v2w; iv3w = 1.f / v3w;
                v1xp = v1x * iv1w; v1yp = v1y * iv1w; v1zp = v1z * iv1w;
                v2xp = v2x * iv2w; v2yp = v2y * iv2w; v2zp = v2z * iv2w;
                v3xp = v3x * iv3w; v3yp = v3y * iv3w; v3zp = v3z * iv3w;
                v1xt = v1xp + width * 0.5f; v1yt = v1yp + height * 0.5f;
                v2xt = v2xp + width * 0.5f; v2yt = v2yp + height * 0.5f;
                v3xt = v3xp + width * 0.5f; v3yt = v3yp + height * 0.5f;
                for(int j = 0; j < this->varyingsPerVertex; j += 1) {
                    varyings1[j] = vertices[v1 + 4 + j];
                    varyings2[j] = vertices[v2 + 4 + j];
                    varyings3[j] = vertices[v3 + 4 + j];
                }
            }
            else if(v1w < CLIP && v2w < CLIP && v3w > CLIP) {
                float t1 = (CLIP - v3w) / (v1w - v3w);
                float t2 = (CLIP - v3w) / (v2w - v3w);
                iv1w = INV_CLIP; iv2w = INV_CLIP; iv3w = 1.f / v3w;
                v1xp = (v3x + (v1x - v3x) * t1) * iv1w; v1yp = (v3y + (v1y - v3y) * t1) * iv1w; v1zp = (v3z + (v1z - v3z) * t1) * iv1w;
                v2xp = (v3x + (v2x - v3x) * t2) * iv2w; v2yp = (v3y + (v2y - v3y) * t2) * iv2w; v2zp = (v3z + (v2z - v3z) * t2) * iv2w;
                v3xp = v3x * iv3w; v3yp = v3y * iv3w; v3zp = v3z * iv3w;
                v1xt = v1xp + width * 0.5; v1yt = v1yp + height * 0.5;
                v2xt = v2xp + width * 0.5; v2yt = v2yp + height * 0.5;
                v3xt = v3xp + width * 0.5; v3yt = v3yp + height * 0.5;
                for(int j = 0; j < this->varyingsPerVertex; j += 1) {
                    varyings1[j] = vertices[v3 + 4 + j] + (vertices[v1 + 4 + j] - vertices[v3 + 4 + j]) * t1;
                    varyings2[j] = vertices[v3 + 4 + j] + (vertices[v2 + 4 + j] - vertices[v3 + 4 + j]) * t2;
                    varyings3[j] = vertices[v3 + 4 + j];
                }
            }
            else if(v1w < CLIP && v2w > CLIP && v3w < CLIP) {
                float t1 = (CLIP - v2w) / (v1w - v2w);
                float t2 = (CLIP - v2w) / (v3w - v2w);
                iv1w = INV_CLIP; iv2w = 1.f / v2w; iv3w = INV_CLIP;
                v1xp = (v2x + (v1x - v2x) * t1) * iv1w; v1yp = (v2y + (v1y - v2y) * t1) * iv1w; v1zp = (v2z + (v1z - v2z) * t1) * iv1w;
                v2xp = v2x * iv2w; v2yp = v2y * iv2w; v2zp = v2z * iv2w;
                v3xp = (v2x + (v3x - v2x) * t2) * iv3w; v3yp = (v2y + (v3y - v2y) * t2) * iv3w; v3zp = (v2z + (v3z - v2z) * t2) * iv3w;
                v1xt = v1xp + width * 0.5; v1yt = v1yp + height * 0.5;
                v2xt = v2xp + width * 0.5; v2yt = v2yp + height * 0.5;
                v3xt = v3xp + width * 0.5; v3yt = v3yp + height * 0.5;
                for(int j = 0; j < this->varyingsPerVertex; j += 1) {
                    varyings1[j] = vertices[v2 + 4 + j] + (vertices[v1 + 4 + j] - vertices[v2 + 4 + j]) * t1;
                    varyings2[j] = vertices[v2 + 4 + j];
                    varyings3[j] = vertices[v2 + 4 + j] + (vertices[v3 + 4 + j] - vertices[v2 + 4 + j]) * t2;
                }
            }
            else if(v1w > CLIP && v2w < CLIP && v3w < CLIP) {
                float t1 = (CLIP - v1w) / (v2w - v1w);
                float t2 = (CLIP - v1w) / (v3w - v1w);
                iv1w = 1.f / v1w; iv2w = INV_CLIP; iv3w = INV_CLIP;
                v1xp = v1x * iv1w; v1yp = v1y * iv1w; v1zp = v1z * iv1w;
                v2xp = (v1x + (v2x - v1x) * t1) * iv2w; v2yp = (v1y + (v2y - v1y) * t1) * iv2w; v2zp = (v1z + (v2z - v1z) * t1) * iv2w;
                v3xp = (v1x + (v3x - v1x) * t2) * iv3w; v3yp = (v1y + (v3y - v1y) * t2) * iv3w; v3zp = (v1z + (v3z - v1z) * t2) * iv3w;
                v1xt = v1xp + width * 0.5; v1yt = v1yp + height * 0.5;
                v2xt = v2xp + width * 0.5; v2yt = v2yp + height * 0.5;
                v3xt = v3xp + width * 0.5; v3yt = v3yp + height * 0.5;
                for(int j = 0; j < this->varyingsPerVertex; j += 1) {
                    varyings1[j] = vertices[v1 + 4 + j];
                    varyings2[j] = vertices[v1 + 4 + j] + (vertices[v2 + 4 + j] - vertices[v1 + 4 + j]) * t1;
                    varyings3[j] = vertices[v1 + 4 + j] + (vertices[v3 + 4 + j] - vertices[v1 + 4 + j]) * t2;
                }
            }
            else if(v1w > CLIP && v2w > CLIP && v3w < CLIP) {
                float t = (CLIP - v3w) / (v1w - v3w);
                iv1w = 1.f / v1w; iv2w = 1.f / v2w; iv3w = INV_CLIP;
                v1xp = v1x * iv1w; v1yp = v1y * iv1w; v1zp = v1z * iv1w;
                v2xp = v2x * iv2w; v2yp = v2y * iv2w; v2zp = v2z * iv2w;
                v3xp = (v3x + (v1x - v3x) * t) * iv3w; v3yp = (v3y + (v1y - v3y) * t) * iv3w; v3zp = (v3z + (v1z - v3z) * t) * iv3w;
                v1xt = v1xp + width * 0.5; v1yt = v1yp + height * 0.5;
                v2xt = v2xp + width * 0.5; v2yt = v2yp + height * 0.5;
                v3xt = v3xp + width * 0.5; v3yt = v3yp + height * 0.5;
                for(int j = 0; j < this->varyingsPerVertex; j += 1) {
                    varyings1[j] = vertices[v1 + 4 + j];
                    varyings2[j] = vertices[v2 + 4 + j];
                    varyings3[j] = vertices[v3 + 4 + j] + (vertices[v1 + 4 + j] - vertices[v3 + 4 + j]) * t;
                }
                clipIntoTwo = true;
            }
            else if(v1w > CLIP && v2w < CLIP && v3w > CLIP) {
                float t = (CLIP - v2w) / (v1w - v2w);
                iv1w = 1.f / v1w; iv2w = INV_CLIP; iv3w = 1.f / v3w;
                v1xp = v1x * iv1w; v1yp = v1y * iv1w; v1zp = v1z * iv1w;
                v2xp = (v2x + (v1x - v2x) * t) * iv2w; v2yp = (v2y + (v1y - v2y) * t) * iv2w; v2zp = (v2z + (v1z - v2z) * t) * iv2w;
                v3xp = v3x * iv3w; v3yp = v3y * iv3w; v3zp = v3z * iv3w;
                v1xt = v1xp + width * 0.5; v1yt = v1yp + height * 0.5;
                v2xt = v2xp + width * 0.5; v2yt = v2yp + height * 0.5;
                v3xt = v3xp + width * 0.5; v3yt = v3yp + height * 0.5;
                for(int j = 0; j < this->varyingsPerVertex; j += 1) {
                    varyings1[j] = vertices[v1 + 4 + j];
                    varyings2[j] = vertices[v2 + 4 + j] + (vertices[v1 + 4 + j] - vertices[v2 + 4 + j]) * t;
                    varyings3[j] = vertices[v3 + 4 + j];
                }
                clipIntoTwo = true;
            }
            else if(v1w < CLIP && v2w > CLIP && v3w > CLIP) {
                float t = (CLIP - v1w) / (v2w - v1w);
                iv1w = INV_CLIP; iv2w = 1.f / v2w; iv3w = 1.f / v3w;
                v1xp = (v1x + (v2x - v1x) * t) * iv1w; v1yp = (v1y + (v2y - v1y) * t) * iv1w; v1zp = (v1z + (v2z - v1z) * t) * iv1w;
                v2xp = v2x * iv2w; v2yp = v2y * iv2w; v2zp = v2z * iv2w;
                v3xp = v3x * iv3w; v3yp = v3y * iv3w; v3zp = v3z * iv3w;
                v1xt = v1xp + width * 0.5; v1yt = v1yp + height * 0.5;
                v2xt = v2xp + width * 0.5; v2yt = v2yp + height * 0.5;
                v3xt = v3xp + width * 0.5; v3yt = v3yp + height * 0.5;
                for(int j = 0; j < this->varyingsPerVertex; j += 1) {
                    varyings1[j] = vertices[v1 + 4 + j] + (vertices[v2 + 4 + j] - vertices[v1 + 4 + j]) * t;
                    varyings2[j] = vertices[v2 + 4 + j];
                    varyings3[j] = vertices[v3 + 4 + j];
                }
                clipIntoTwo = true;
            }
            this->renderTriangle(v1xt, v1yt, v2xt, v2yt, v3xt, v3yt, varyings, varyings1, varyings2, varyings3, width, height, iv1w, iv2w, iv3w, v1zp, v2zp, v3zp, out, fragment, depth);
            if(clipIntoTwo) {
                if(v1w > CLIP && v2w > CLIP && v3w < CLIP) {
                    float t = (CLIP - v3w) / (v2w - v3w);
                    iv1w = INV_CLIP;
                    v1xp = (v3x + (v2x - v3x) * t) * iv1w; v1yp = (v3y + (v2y - v3y) * t) * iv1w; v1zp = (v3z + (v2z - v3z) * t) * iv1w;
                    v1xt = v1xp + width * 0.5; v1yt = v1yp + height * 0.5;
                    for(int j = 0; j < this->varyingsPerVertex; j += 1) {
                        varyings1[j] = vertices[v3 + 4 + j] + (vertices[v2 + 4 + j] - vertices[v3 + 4 + j]) * t;
                    }
                }
                else if(v1w > CLIP && v2w < CLIP && v3w > CLIP) {
                    float t = (CLIP - v2w) / (v3w - v2w);
                    iv1w = INV_CLIP;
                    v1xp = (v2x + (v3x - v2x) * t) * iv1w; v1yp = (v2y + (v3y - v2y) * t) * iv1w; v1zp = (v2z + (v3z - v2z) * t) * iv1w;
                    v1xt = v1xp + width * 0.5; v1yt = v1yp + height * 0.5;
                    for(int j = 0; j < this->varyingsPerVertex; j += 1) {
                        varyings1[j] = vertices[v2 + 4 + j] + (vertices[v3 + 4 + j] - vertices[v2 + 4 + j]) * t;
                    }
                }
                else if(v1w < CLIP && v2w > CLIP && v3w > CLIP) {
                    float t = (CLIP - v1w) / (v3w - v1w);
                    iv2w = INV_CLIP;
                    v2xp = (v1x + (v3x - v1x) * t) * iv2w; v2yp = (v1y + (v3y - v1y) * t) * iv2w; v2zp = (v1z + (v3z - v1z) * t) * iv2w;
                    v2xt = v2xp + width * 0.5; v2yt = v2yp + height * 0.5;
                    for(int j = 0; j < this->varyingsPerVertex; j += 1) {
                        varyings2[j] = vertices[v1 + 4 + j] + (vertices[v3 + 4 + j] - vertices[v1 + 4 + j]) * t;
                    }
                }
                this->renderTriangle(v1xt, v1yt, v2xt, v2yt, v3xt, v3yt, varyings, varyings1, varyings2, varyings3, width, height, iv1w, iv2w, iv3w, v1zp, v2zp, v3zp, out, fragment, depth);
            }
        }
    }
}
