/*
Copyright 2018 Oscar Sebio Cajaraville

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "fornosui.h"
#include "fornos.h"
#include "logging.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <imguifilesystem.h>
#include <imgui_impl_glfw_gl3.h>
#include <glfw/glfw3.h>
#include <string>
#include <cstdlib>

static const char* normalImportNames[3] = { "Import", "Compute per face", "Compute per vertex" };
static const char* meshMappingMethodNames[3] = { "Smooth", "Low-poly normals", "Hybrid" };

inline void SetupImGuiStyle(bool bStyleDark_, float alpha_)
{
	ImGuiStyle& style = ImGui::GetStyle();

	// light style from Pacôme Danhiez (user itamago) https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
	style.Alpha = 1.0f;
	style.FrameRounding = 3.0f;
	style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
	style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Column] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
	style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	style.Colors[ImGuiCol_CloseButton] = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
	style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

	if (bStyleDark_)
	{
		for (int i = 0; i <= ImGuiCol_COUNT; i++)
		{
			ImVec4& col = style.Colors[i];
			float H, S, V;
			ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);

			if (S < 0.1f)
			{
				V = 1.0f - V;
			}
			ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
			if (col.w < 1.00f)
			{
				col.w *= alpha_;
			}
		}
	}
	else
	{
		for (int i = 0; i <= ImGuiCol_COUNT; i++)
		{
			ImVec4& col = style.Colors[i];
			if (col.w < 1.00f)
			{
				col.x *= alpha_;
				col.y *= alpha_;
				col.z *= alpha_;
				col.w *= alpha_;
			}
		}
	}
}

static void ShowHelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

static void parameters_begin()
{
	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, 130);
}

static void parameters_end()
{
	ImGui::Columns(1);
}

static void parameter_common(const char *name, const char *help)
{
	ImGui::Text(name);
	ImGui::SameLine();
	ShowHelpMarker(help);
	ImGui::NextColumn();
}

static void parameter(const char *name, int *value, const char *id, const char *help)
{
	parameter_common(name, help);
	ImGui::InputInt(id, value);
	ImGui::NextColumn();
}

static void parameter(const char *name, float *value, const char *id, const char *help)
{
	parameter_common(name, help);
	ImGui::InputFloat(id, value);
	ImGui::NextColumn();
}

static void parameter(const char *name, bool *value, const char *id, const char *help)
{
	parameter_common(name, help);
	ImGui::Checkbox(id, value);
	ImGui::NextColumn();
}

static void parameter_texSize(const char *name, int *width, int *height, const char *id, const char *help)
{
	parameter_common(name, help);
	ImGui::PushItemWidth(100);
	ImGui::DragInt("##TexW", width, 32.0f, 256, 8192);
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::Text("x");
	ImGui::SameLine();
	ImGui::PushItemWidth(100);
	ImGui::DragInt("##TexH", height, 32.0f, 256, 8192);
	ImGui::PopItemWidth();
	ImGui::NextColumn();
}

class PathField
{
public:
	PathField(std::string *path) : _path(path) {}

	void draw(const char *title, const char *extensionFilter, bool save, ImVec2 size)
	{
		strncpy_s(s_buff, 2048, _path->c_str(), _path->size());
		if (ImGui::InputText("##path", s_buff, 2048))
		{
			*_path = std::string(s_buff);
		}
		ImGui::SameLine();
		const bool pressed = ImGui::Button("...##path");

		const char *path_str = nullptr;

		if (save)
		{
			path_str = _fsDialog.saveFileDialog
			(
				pressed,
				_path->empty() ? s_fsPath.c_str() : _path->c_str(), "",
				extensionFilter, title,
				size, ImVec2(0, 0)
			);
		}
		else
		{
			path_str = _fsDialog.chooseFileDialog
			(
				pressed,
				_path->empty() ? s_fsPath.c_str() : _path->c_str(),
				extensionFilter, title,
				size, ImVec2(0, 0)
			);
		}

		if (strlen(path_str) != 0)
		{
			*_path = std::string(path_str);
			s_fsPath = *_path;
		}
	}

private:
	static std::string s_fsPath;
	static char s_buff[2048];
	ImGuiFs::Dialog _fsDialog;
	std::string *_path;
};

std::string PathField::s_fsPath;
char PathField::s_buff[2048];

static ImGuiFs::Dialog s_fsDialog;
static std::string s_fsPath;

static void parameter_openFile
(
	const char *name,
	PathField *path,
	const char *id,
	const char *help,
	const char *title,
	const char *extensionFilter,
	int w, int h
)
{
	ImGui::PushID(id);
	parameter_common(name, help);
	path->draw(title, extensionFilter, false, ImVec2(float(w), float(h)));
	ImGui::NextColumn();
	ImGui::PopID();
}

static void parameter_saveFile
(
	const char *name,
	PathField *path,
	const char *id,
	const char *help,
	const char *title,
	const char *extensionFilter,
	int w, int h
)
{
	ImGui::PushID(id);
	parameter_common(name, help);
	path->draw(title, extensionFilter, true, ImVec2(float(w), float(h)));
	ImGui::NextColumn();
	ImGui::PopID();
}

static void parameter(const char *name, size_t *v, const char *enumNames[], const size_t enumNamesCount, const char *id, const char *help)
{
	parameter_common(name, help);
	ImGui::PushID(id);
	if (ImGui::Button(enumNames[*v]))
	{
		ImGui::OpenPopup("EnumPopup");
	}

	if (ImGui::BeginPopup("EnumPopup"))
	{
		for (size_t i = 0; i < enumNamesCount; ++i)
		{
			if (ImGui::Selectable(enumNames[i]))
			{
				*v = i;
			}
		}
		ImGui::EndPopup();
	}
	ImGui::NextColumn();
	ImGui::PopID();
}

template <typename T>
inline void parameter(const char *name, T *v, const char *enumNames[], const size_t enumNamesCount, const char *id, const char *help)
{
	size_t v1 = size_t(*v);
	parameter(name, &v1, enumNames, enumNamesCount, id, help);
	*v = T(v1);
}

class FornosParameters_Shared_View
{
public:
	FornosParameters_Shared_View(FornosParameters_Shared *data)
		: data(data)
		, hiPolyPath(&data->hiPolyMeshPath)
		, loPolyPath(&data->loPolyMeshPath)
	{
	}

	void render(int windowWidth, int windowHeight);
private:
	FornosParameters_Shared *data;
	PathField loPolyPath;
	PathField hiPolyPath;
};

void FornosParameters_Shared_View::render(int windowWidth, int windowHeight)
{
	parameters_begin();

	parameter_openFile("Low Mesh", &loPolyPath, "##lo",
		"Low resolution mesh file.\n"
		"Wavefront OBJ files supported.",
		"Select Low-Poly Mesh", ".obj",
		windowWidth, windowHeight);

	parameter<NormalImport>("Normals", &data->loPolyMeshNormal, normalImportNames, 3, "#lowPolyNormal",
		"How the model normals are imported or computed.");

	parameter_openFile("High Mesh", &hiPolyPath, "##hi",
		"Optional high resolution mesh file.\n"
		"If not setup it will bake the low resolution mesh.\n"
		"Wavefront OBJ files supported.",
		"Select Hiigh-Poly Mesh", ".obj",
		windowWidth, windowHeight);

	parameter<NormalImport>("Normals", &data->hiPolyMeshNormal, normalImportNames, 3, "#hiPolyNormal",
		"How the model normals are imported or computed.");

	parameter_texSize("Tex Size", &data->texWidth, &data->texHeight, "#texSize",
		"Texture output size (width x height).\n"
		"Control+click to edit the number.");

	parameter("Tex Dilation", &data->texDilation, "##texDilation",
		"Fills empty areas of the generated image with neighbour pixels.\n"
		"This value is the distance (in pixels) for searching a pixel with data to use.\n"
		"A value of zero will produce no dilation.");

	parameter<MeshMappingMethod>("Mapping method", &data->mapping, meshMappingMethodNames, 3, "#meshMapping",
		"How rays are generated to map the low-poly mesh to the high-poly mesh.\n"
		"Smooth creates continuous direction for the rays.\n"
		"Low-poly normals uses the normal direction imported or computed for the low-poly mesh");

	if (data->mapping == MeshMappingMethod::Hybrid)
	{
		parameter("Mapping edge", &data->mappingEdge, "##mappingEdge",
			"Distance to sharp edges to start auto smoothing for mesh mapping");
	}

	parameter("Ignore backfaces", &data->ignoreBackfaces, "##ignoreBackface",
		"If checked faces on the oposite direction to the mesh-mapping rays will be ignored during mesh mapping.");

	parameter("BVH Tri. Count", &data->bvhTrisPerNode, "##BvhTriCount",
		"Maximum number of triangles per BVH leaf node.");

	parameters_end();
}

class FornosParameters_SolverHeight_View
{
public:
	FornosParameters_SolverHeight_View(FornosParameters_SolverHeight *data)
		: data(data)
		, path(&data->outputPath)
	{
	}

	void render(int windowWidth, int windowHeight);
private:
	FornosParameters_SolverHeight *data;
	PathField path;
};

void FornosParameters_SolverHeight_View::render(int windowWidth, int windowHeight)
{
	ImGui::PushID("HeightsSolver");

	bool expanded = ImGui::CollapsingHeader("Height", ImGuiTreeNodeFlags_AllowItemOverlap);
	ImGui::SameLine(ImGui::GetWindowWidth() - 30);
	ImGui::Checkbox("", &data->enabled);
	if (expanded)
	{
		if (!data->enabled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		parameters_begin();

		parameter_saveFile("Output", &path, "##height",
			"Height image output file.",
			"Height Map", ".png;.tga;.exr",
			windowWidth, windowHeight);

		parameters_end();

		if (!data->enabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}

	ImGui::PopID();
}

class FornosParameters_SolverPositions_View
{
public:
	FornosParameters_SolverPositions_View(FornosParameters_SolverPositions *data)
		: data(data)
		, path(&data->outputPath)
	{
	}

	void render(int windowWidth, int windowHeight);
private:
	FornosParameters_SolverPositions *data;
	PathField path;
};

void FornosParameters_SolverPositions_View::render(int windowWidth, int windowHeight)
{
	ImGui::PushID("PositionsSolver");

	bool expanded = ImGui::CollapsingHeader("Position", ImGuiTreeNodeFlags_AllowItemOverlap);
	ImGui::SameLine(ImGui::GetWindowWidth() - 30);
	ImGui::Checkbox("", &data->enabled);
	if (expanded)
	{
		if (!data->enabled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		parameters_begin();

		parameter_saveFile("Output", &path, "##pos",
			"Positions image output file.",
			"Positions Map", ".png;.tga;.exr",
			windowWidth, windowHeight);

		parameters_end();

		if (!data->enabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}

	ImGui::PopID();
}

class FornosParameters_SolverNormals_View
{
public:
	FornosParameters_SolverNormals_View(FornosParameters_SolverNormals *data)
		: data(data)
		, path(&data->outputPath)
	{
	}

	void render(int windowWidth, int windowHeight);
private:
	FornosParameters_SolverNormals *data;
	PathField path;
};

void FornosParameters_SolverNormals_View::render(int windowWidth, int windowHeight)
{
	ImGui::PushID("NormalsSolver");

	bool normalsExpanded = ImGui::CollapsingHeader("Normals", ImGuiTreeNodeFlags_AllowItemOverlap);
	ImGui::SameLine(ImGui::GetWindowWidth() - 30);
	ImGui::Checkbox("", &data->enabled);
	if (normalsExpanded)
	{
		if (!data->enabled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		parameters_begin();

		parameter_saveFile("Output", &path, "##normals",
			"Normal map output file.",
			"Normals Map", ".png;.tga;.exr",
			windowWidth, windowHeight);

		parameter("Tangent space", &data->tangentSpace, "##normalsTanSpace",
			"Computes tangent space normals if this is checked.\n"
			"Otherwise it generates object space normals.");

		parameters_end();

		if (!data->enabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}

	ImGui::PopID();
}

class FornosParameters_SolverAO_View
{
public:
	FornosParameters_SolverAO_View(FornosParameters_SolverAO *data)
		: data(data)
		, path(&data->outputPath)
	{
	}

	void render(int windowWidth, int windowHeight);
private:
	FornosParameters_SolverAO *data;
	PathField path;
};

void FornosParameters_SolverAO_View::render(int windowWidth, int windowHeight)
{
	ImGui::PushID("AmbientOcclusionSolver");

	bool aoExpanded = ImGui::CollapsingHeader("Ambient Occlusion", ImGuiTreeNodeFlags_AllowItemOverlap);
	ImGui::SameLine(ImGui::GetWindowWidth() - 30);
	ImGui::Checkbox("", &data->enabled);
	if (aoExpanded)
	{
		if (!data->enabled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		parameters_begin();

		parameter_saveFile("Output", &path, "##ao",
			"Ambient occlusion image output file.",
			"Ambient Occlusion Map", ".png;.tga;.exr",
			windowWidth, windowHeight);

		parameter("Sample count", &data->sampleCount, "##aoSampleCount",
			"Number of samples.\nLarger = better & slower.");
		parameter("Min distance", &data->minDistance, "##aoMinDistance",
			"Occluders closer than this value are ignored.");
		parameter("Max distance", &data->maxDistance, "##aoMaxDistance",
			"Max distance to consider occluders.");

		parameters_end();

		if (!data->enabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}

	ImGui::PopID();
}

class FornosParameters_SolverBentNormals_View
{
public:
	FornosParameters_SolverBentNormals_View(FornosParameters_SolverBentNormals *data)
		: data(data)
		, path(&data->outputPath)
	{
	}

	void render(int windowWidth, int windowHeight);
private:
	FornosParameters_SolverBentNormals *data;
	PathField path;
};

void FornosParameters_SolverBentNormals_View::render(int windowWidth, int windowHeight)
{
	ImGui::PushID("BentNormalsSolver");

	bool expanded = ImGui::CollapsingHeader("Bent Normals", ImGuiTreeNodeFlags_AllowItemOverlap);
	ImGui::SameLine(ImGui::GetWindowWidth() - 30);
	ImGui::Checkbox("", &data->enabled);
	if (expanded)
	{
		if (!data->enabled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		parameters_begin();

		parameter_saveFile("Output", &path, "##bn",
			"Bent normals image output file.",
			"Bent Normals Map", ".png;.tga;.exr",
			windowWidth, windowHeight);

		parameter("Sample count", &data->sampleCount, "##bnSampleCount",
			"Number of samples.\nLarger = better & slower.");
		parameter("Min distance", &data->minDistance, "##bnMinDistance",
			"Occluders closer than this value are ignored.");
		parameter("Max distance", &data->maxDistance, "##bnMaxDistance",
			"Occluders farther than this value are ignored.");
		parameter("Tangent space", &data->tangentSpace, "##bnTanSpace",
			"Compute normals in tangent space.");

		parameters_end();

		if (!data->enabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}

	ImGui::PopID();
}

class FornosParameters_SolverThickness_View
{
public:
	FornosParameters_SolverThickness_View(FornosParameters_SolverThickness *data)
		: data(data)
		, path(&data->outputPath)
	{
	}

	void render(int windowWidth, int windowHeight);
private:
	FornosParameters_SolverThickness *data;
	PathField path;
};

void FornosParameters_SolverThickness_View::render(int windowWidth, int windowHeight)
{
	ImGui::PushID("ThicknessSolver");

	bool thicknessExpanded = ImGui::CollapsingHeader("Thickness", ImGuiTreeNodeFlags_AllowItemOverlap);
	ImGui::SameLine(ImGui::GetWindowWidth() - 30);
	ImGui::Checkbox("", &data->enabled);
	if (thicknessExpanded)
	{
		if (!data->enabled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		parameters_begin();

		parameter_saveFile("Output", &path, "##thickness",
			"Thickness image output file.",
			"Thickness Map", ".png;.tga;.exr",
			windowWidth, windowHeight);

		parameter("Sample count", &data->sampleCount, "##thicknessSampleCount",
			"Number of samples.\nLarger = better & slower.");
		parameter("Min distance", &data->minDistance, "##thicknessMinDistance",
			"Collisions closer than this value are ignored.");
		parameter("Max distance", &data->maxDistance, "##thicknessMaxDistance",
			"Full thickness at this distance.");

		parameters_end();

		if (!data->enabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}

	ImGui::PopID();
}

class FornosUI_Impl
{
public:
	FornosUI_Impl();
	void render(int windowWidth, int windowHeight);
	void setRunner(FornosRunner *runner) { _runner = runner;  }

protected:
	void renderMainMenu();
	void renderParameters(int windowWidth, int windowHeight);
	void renderWorkInProgress();
	void renderErrors();
	void renderLogWindow();
	void renderAboutPopup();
	void startBaking();

private:
	FornosParameters _params;
	FornosParameters_Shared_View _paramsSharedView;
	FornosParameters_SolverHeight_View _paramsSolverHeightView;
	FornosParameters_SolverPositions_View _paramsSolverPositionsView;
	FornosParameters_SolverNormals_View _paramsSolverNormalsView;
	FornosParameters_SolverAO_View _paramsSolverAOView;
	FornosParameters_SolverBentNormals_View _paramsSolverBentNormalsView;
	FornosParameters_SolverThickness_View _paramsSolverThicknessView;
	FornosRunner *_runner = nullptr;
	std::string _bakeErrors;

	bool _showLog = false;
	bool _showAbout = false;
	float _mainMenuHeight = 0;
};

FornosUI_Impl::FornosUI_Impl()
	: _paramsSharedView(&_params.shared)
	, _paramsSolverHeightView(&_params.height)
	, _paramsSolverPositionsView(&_params.positions)
	, _paramsSolverNormalsView(&_params.normals)
	, _paramsSolverAOView(&_params.ao)
	, _paramsSolverBentNormalsView(&_params.bentNormals)
	, _paramsSolverThicknessView(&_params.thickness)
{
}

void FornosUI_Impl::render(int windowWidth, int windowHeight)
{
	ImGui_ImplGlfwGL3_NewFrame();

	renderMainMenu();

	const float logWindowHeight = _showLog ? 200.0f : 0.0f;
	const float mainWindowHeight = float(windowHeight) - logWindowHeight - _mainMenuHeight;
	const float mainWindowPosY = _mainMenuHeight;
	const float logWindowPosY = mainWindowHeight + mainWindowPosY;

	ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)mainWindowHeight));
	ImGui::SetNextWindowPos(ImVec2(0.0f, _mainMenuHeight));
	renderParameters(windowWidth, windowHeight);

	ImGui::SetNextWindowSize(ImVec2(float(windowWidth), logWindowHeight));
	ImGui::SetNextWindowPos(ImVec2(0.0f, logWindowPosY));
	renderLogWindow();

	renderWorkInProgress();
	renderErrors();
	renderAboutPopup();
}

void FornosUI_Impl::renderMainMenu()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Window"))
		{
			ImGui::MenuItem("Log", "CTRL+L", &_showLog);
			ImGui::Separator();
			ImGui::MenuItem("About", nullptr, &_showAbout);
			ImGui::EndMenu();
		}
		_mainMenuHeight = ImGui::GetWindowHeight();
		ImGui::EndMainMenuBar();
	}

	if (ImGui::IsKeyPressed(GLFW_KEY_L, false) && ImGui::GetIO().KeyCtrl)
	{
		_showLog = !_showLog;
	}
}

void FornosUI_Impl::renderParameters(int windowWidth, int windowHeight)
{
	ImGui::Begin(
		"Texture baking",
		nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove);

	_paramsSharedView.render(windowWidth, windowHeight);
	_paramsSolverHeightView.render(windowWidth, windowHeight);
	_paramsSolverPositionsView.render(windowWidth, windowHeight);
	_paramsSolverNormalsView.render(windowWidth, windowHeight);
	_paramsSolverAOView.render(windowWidth, windowHeight);
	_paramsSolverBentNormalsView.render(windowWidth, windowHeight);
	_paramsSolverThicknessView.render(windowWidth, windowHeight);

	const bool readyToBake =
		_params.thickness.ready() ||
		_params.height.ready() ||
		_params.positions.ready() ||
		_params.normals.ready() ||
		_params.ao.ready() ||
		_params.thickness.ready();

	if (!readyToBake)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}

	if (ImGui::Button("Bake"))
	{
		_bakeErrors.clear();
		startBaking();
		if (!_bakeErrors.empty())
		{
			ImGui::OpenPopup("ErrorsPopup");
		}
	}

	if (!readyToBake)
	{
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	}

	ImGui::End();
}

void FornosUI_Impl::renderWorkInProgress()
{
	if (_runner->pending())
	{
		if (!ImGui::IsPopupOpen("TaskPopup"))
		{
			ImGui::OpenPopup("TaskPopup");
		}
	}

	ImGui::SetNextWindowSizeConstraints(ImVec2(300, 40), ImVec2(500, 500));
	if (ImGui::BeginPopupModal("TaskPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		if (!_runner->pending())
		{
			ImGui::CloseCurrentPopup();
		}
		else
		{
			ImGui::Text("Baking");
			ImGui::SameLine();
			auto task = _runner->currentTask();
			ImGui::Text(task->name());
			ImGui::ProgressBar(task->progress());
		}
		ImGui::EndPopup();
	}
}

void FornosUI_Impl::renderErrors()
{
	if (ImGui::BeginPopupModal("ErrorsPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::Text("Baking errors:");
		ImGui::Text(_bakeErrors.c_str());
		if (ImGui::Button("Ok"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void FornosUI_Impl::renderLogWindow()
{
	ImGui::Begin("Log", &_showLog,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove);
	ImGui::TextUnformatted(getLogBuffer().c_str());
	ImGui::SetScrollHere(1.0f);
	ImGui::End();
}

void FornosUI_Impl::renderAboutPopup()
{
	if (_showAbout && !ImGui::IsPopupOpen("AboutPopup"))
	{
		ImGui::OpenPopup("AboutPopup");
	}

	if (ImGui::BeginPopupModal("AboutPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::Text("Fornos");
		ImGui::Text("v0.1.1");
		ImGui::Text("Oscar S.C. (c) 2018");
		if (ImGui::Button("Ok"))
		{
			ImGui::CloseCurrentPopup();
			_showAbout = false;
		}
		ImGui::EndPopup();
	}
}

void FornosUI_Impl::startBaking()
{
	_runner->start(_params, _bakeErrors);
}

FornosUI::FornosUI()
	: _impl(new FornosUI_Impl())
{
}

FornosUI::~FornosUI()
{
}

void FornosUI::init(FornosRunner *runner, GLFWwindow *window)
{
	_impl->setRunner(runner);
	ImGui_ImplGlfwGL3_Init(window, true);
	
	SetupImGuiStyle(true, 1.0f);

	ImGuiIO& io = ImGui::GetIO();

#if _WIN32
	const std::string windir = std::getenv("windir");
	const std::string fontdir = windir + "\\fonts\\segoeui.ttf";
	io.Fonts->AddFontFromFileTTF(fontdir.c_str(), 16.0f);
#endif
}

void FornosUI::shutdown()
{
	ImGui_ImplGlfwGL3_Shutdown();
}

void FornosUI::process(int windowWidth, int windowHeight)
{
	_impl->render(windowWidth, windowHeight);
}

void FornosUI::render()
{
	ImGui::Render();
}

