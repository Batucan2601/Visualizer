#include "raylib.h"
#include "raymath.h"
#include <fstream>
#include <sstream>
#include <vector>

void ComputeMeshNormals(Mesh* mesh) {
    int vCount = mesh->vertexCount;
    int tCount = mesh->triangleCount;
    float* verts = mesh->vertices;
    unsigned short* indices = mesh->indices;

    // Allocate normals array
    mesh->normals = (float*)MemAlloc(vCount * 3 * sizeof(float));
    for (int i = 0; i < vCount * 3; i++) mesh->normals[i] = 0.0f;

    // Accumulate face normals per vertex
    for (int i = 0; i < tCount; i++) {
        int idx0 = indices[i * 3 + 0];
        int idx1 = indices[i * 3 + 1];
        int idx2 = indices[i * 3 + 2];

        Vector3 v0 = { verts[idx0 * 3 + 0], verts[idx0 * 3 + 1], verts[idx0 * 3 + 2] };
        Vector3 v1 = { verts[idx1 * 3 + 0], verts[idx1 * 3 + 1], verts[idx1 * 3 + 2] };
        Vector3 v2 = { verts[idx2 * 3 + 0], verts[idx2 * 3 + 1], verts[idx2 * 3 + 2] };

        Vector3 edge1 = Vector3Subtract(v1, v0);
        Vector3 edge2 = Vector3Subtract(v2, v0);
        Vector3 faceNormal = Vector3CrossProduct(edge1, edge2);
        faceNormal = Vector3Normalize(faceNormal);

        // Add to each vertex normal
        for (int j = 0; j < 3; j++) {
            int idx = indices[i * 3 + j];
            mesh->normals[idx * 3 + 0] += faceNormal.x;
            mesh->normals[idx * 3 + 1] += faceNormal.y;
            mesh->normals[idx * 3 + 2] += faceNormal.z;
        }
    }

    // Normalize vertex normals
    for (int i = 0; i < vCount; i++) {
        Vector3 n = {
            mesh->normals[i * 3 + 0],
            mesh->normals[i * 3 + 1],
            mesh->normals[i * 3 + 2]
        };
        n = Vector3Normalize(n);
        mesh->normals[i * 3 + 0] = n.x;
        mesh->normals[i * 3 + 1] = n.y;
        mesh->normals[i * 3 + 2] = n.z;
    }

    UploadMesh(mesh, false);  // Re-upload with normals
}


void NormalizeMesh(Mesh* mesh) {
    int vCount = mesh->vertexCount;
    float* verts = mesh->vertices;

    Vector3 min = { 0, 0, 0 };
    Vector3 max = { 0, 0, 0 };

    // Find bounding box
    for (int i = 0; i < vCount; i++) {
        Vector3 v = { verts[i*3 + 0], verts[i*3 + 1], verts[i*3 + 2] };
        min = Vector3Min(min, v);
        max = Vector3Max(max, v);
    }

    Vector3 center = Vector3Scale(Vector3Add(min, max), 0.5f);
    float scale = 1.0f / Vector3Length(Vector3Subtract(max, min));

    // Normalize vertices
    for (int i = 0; i < vCount; i++) {
        Vector3 v = { verts[i*3 + 0], verts[i*3 + 1], verts[i*3 + 2] };
        v = Vector3Subtract(v, center);
        v = Vector3Scale(v, scale);
        verts[i*3 + 0] = v.x;
        verts[i*3 + 1] = v.y;
        verts[i*3 + 2] = v.z;
    }

    UploadMesh(mesh, false);  // Re-upload normalized data
}

Mesh LoadMeshFromOFF(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open .off file: %s", filename);
        return Mesh{};
    }

    std::string line;
    std::getline(file, line);
    if (line != "OFF") {
        TraceLog(LOG_ERROR, "Invalid OFF file header.");
        return Mesh{};
    }

    int numVertices, numFaces, numEdges;
    file >> numVertices >> numFaces >> numEdges;

    std::vector<Vector3> vertices(numVertices);
    for (int i = 0; i < numVertices; i++) {
        file >> vertices[i].x >> vertices[i].y >> vertices[i].z;
    }

    std::vector<unsigned int> indices;
    for (int i = 0; i < numFaces; i++) {
        int vertsPerFace;
        file >> vertsPerFace;
        if (vertsPerFace != 3) {
            TraceLog(LOG_WARNING, "Non-triangle face detected. Skipping.");
            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        unsigned int a, b, c;
        file >> a >> b >> c;
        indices.push_back(a);
        indices.push_back(b);
        indices.push_back(c);
    }

    // Convert to Raylib mesh
    Mesh mesh = { 0 };
    mesh.vertexCount = (int)vertices.size();
    mesh.triangleCount = (int)(indices.size() / 3);

    // Allocate and copy vertex data
    mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    for (int i = 0; i < mesh.vertexCount; i++) {
        mesh.vertices[i * 3 + 0] = vertices[i].x;
        mesh.vertices[i * 3 + 1] = vertices[i].y;
        mesh.vertices[i * 3 + 2] = vertices[i].z;
    }

    mesh.indices = (unsigned short*)MemAlloc(indices.size() * sizeof(unsigned short));
    for (size_t i = 0; i < indices.size(); i++) {
        mesh.indices[i] = (unsigned short)indices[i];
    }
    ComputeMeshNormals(&mesh);  // Raylib will generate vertex normals
    NormalizeMesh(&mesh);
    UploadMesh(&mesh, false);
    return mesh;
}


int main() {
    TraceLog(LOG_INFO, "Raylib version: %i.%i", RAYLIB_VERSION_MAJOR, RAYLIB_VERSION_MINOR);
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Raylib Mesh Viewer with Drag & Drop");


    float rotationX = 0.0f;
    float rotationY = 0.0f;
    Vector2 prevMouse = GetMousePosition();
    Shader shader = LoadShader("shaders/meshVertex.vs", "shaders/meshFragment.fs");
    if( shader.id == 0 ) {
        TraceLog(LOG_ERROR, "Failed to load shader");
        CloseWindow();
        return -1;
    }
    // Send uniforms (get locations once)
    int mvpLoc      = GetShaderLocation(shader, "mvp");
    int modelLoc    = GetShaderLocation(shader, "matModel");
    int normalLoc   = GetShaderLocation(shader, "matNormal");
    int lightLoc    = GetShaderLocation(shader, "lightPos");
    int colorLoc    = GetShaderLocation(shader, "baseColor");
    // Set light position and base color
    Vector3 light = { 5.0f, 5.0f, 5.0f };
    SetShaderValue(shader, lightLoc, &light, SHADER_UNIFORM_VEC3);
    Color baseColor = LIGHTGRAY;
    float color[4] = { baseColor.r/255.0f, baseColor.g/255.0f, baseColor.b/255.0f, 1.0f };
    SetShaderValue(shader, colorLoc, color, SHADER_UNIFORM_VEC4);


    Camera3D camera = {
        .position = { 0.0f, 0.0f, 3.0f },
        .target = { 0.0f, 0.0f, 0.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE
    };

    Model model = { 0 };
    bool modelLoaded = false;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            rotationY += (mouse.x - prevMouse.x) * 0.4f;
            rotationX += (mouse.y - prevMouse.y) * 0.4f;
        }
        prevMouse = mouse;

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);


        Matrix modelMatrix = model.transform;
        Matrix normalMatrix = MatrixTranspose(MatrixInvert(modelMatrix));

        Matrix view = GetCameraMatrix(camera);
        Matrix projection = MatrixPerspective(DEG2RAD * camera.fovy, (float)GetScreenWidth() / GetScreenHeight(), 0.1f, 100.0f);
        Matrix mvp = MatrixMultiply(MatrixMultiply(modelMatrix, view), projection);

        SetShaderValueMatrix(shader, mvpLoc, mvp);
        SetShaderValueMatrix(shader, modelLoc, modelMatrix);
        SetShaderValueMatrix(shader, normalLoc, normalMatrix);
        if (modelLoaded) 
        {
            Matrix rotX = MatrixRotateX(DEG2RAD * rotationX);
            Matrix rotY = MatrixRotateY(DEG2RAD * rotationY);
            model.transform = MatrixMultiply(rotX, rotY);
            DrawModel(model, { 0, 0, 0 }, 1.0f, LIGHTGRAY);
        }       
        else {
        }


        EndMode3D();

        DrawText("Drag and drop a .obj file to load a mesh.", 10, 10, 20, DARKGRAY);
        DrawFPS(screenWidth - 90, 10);
        EndDrawing();

        if (IsFileDropped()) 
        {
            FilePathList dropped = LoadDroppedFiles();
            const char* path = dropped.paths[0];

            if (IsFileExtension(path, ".off")) {
                if (modelLoaded) UnloadModel(model);
                Mesh mesh = LoadMeshFromOFF(path);
                model = LoadModelFromMesh(mesh);
                modelLoaded = true;
                model.materials[0].shader = shader;
            }

           UnloadDroppedFiles(dropped);
        }

    }

    if (modelLoaded) UnloadModel(model);
    CloseWindow();
    return 0;
}
