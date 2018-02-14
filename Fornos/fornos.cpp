
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw_gl3.h>
#include <imguifilesystem.h>
#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <memory>

#include "bvh.h"
#include "compute.h"
#include "mesh.h"
#include "meshmapping.h"
#include "solver_ao.h"
#include "solver_bentnormals.h"
#include "solver_normals.h"
#include "solver_thickness.h"

#include <filesystem>

static int windowWidth = 640;
static int windowHeight = 480;
static std::vector<FornosTask*> tasks;

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error %d: %s\n", error, description);
}

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

class PathField
{
public:
	void draw(const char *title, const char *extensionFilter, bool save)
	{
		strncpy_s(s_buff, 2048, _path.c_str(), _path.size());
		if (ImGui::InputText("##path", s_buff, 2048))
		{
			_path = std::string(s_buff);
		}
		ImGui::SameLine();
		const bool pressed = ImGui::Button("...##path");

		const char *path_str = nullptr;
		
		if (save)
		{
			path_str = _fsDialog.saveFileDialog
			(
				pressed,
				_path.empty() ? s_fsPath.c_str() : _path.c_str(), "",
				extensionFilter, title,
				ImVec2((float)windowWidth, (float)windowHeight),
				ImVec2(0, 0)
			);
		}
		else
		{
			path_str = _fsDialog.chooseFileDialog
			(
				pressed,
				_path.empty() ? s_fsPath.c_str() : _path.c_str(),
				extensionFilter, title,
				ImVec2((float)windowWidth, (float)windowHeight),
				ImVec2(0, 0)
			);
		}

		if (strlen(path_str) != 0)
		{
			_path = std::string(path_str);
			s_fsPath = _path;
		}
	}

	const std::string& get() const { return _path; }

private:
	static std::string s_fsPath;
	static char s_buff[2048];
	ImGuiFs::Dialog _fsDialog;
	std::string _path;
};

std::string PathField::s_fsPath;
char PathField::s_buff[2048];

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

static ImGuiFs::Dialog s_fsDialog;
static std::string s_fsPath;

static void parameter_openFile
(
	const char *name, 
	PathField *path, 
	const char *id, 
	const char *help,
	const char *title,
	const char *extensionFilter
)
{
	ImGui::PushID(id);
	parameter_common(name, help);
	path->draw(title, extensionFilter, false);
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
	const char *extensionFilter
)
{
	ImGui::PushID(id);
	parameter_common(name, help);
	path->draw(title, extensionFilter, true);
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

class FornosUI
{
public:
	FornosUI();
	void render();

protected:
	void renderParameters();
	void renderParamsShared();
	void renderParamsSolverNormals();
	void renderParamsSolverAO();
	void renderParamsSolverBentNormals();
	void renderParamsSolverThickness();
	void renderWorkInProgress();
	void renderErrors();
	void startBaking();

private:
	enum NormalImport { Import = 0, ComputePerFace = 1, ComputePerVertex = 2 };

	// Mesh data
	PathField _lowPolyPath;
	PathField _hiPolyPath;
	NormalImport _lowPolyNormal;
	NormalImport _hiPolyNormal;
	int _bvhTrisPerNode;

	// Texture data
	int _texWidth;
	int _texHeight;

	// Normals solver data
	bool _normals_enabled;
	bool _normals_tangentSpace;
	PathField _normals_outputPath;

	// AO solver data
	bool _ao_enabled;
	int _ao_sampleCount;
	float _ao_minDistance;
	float _ao_maxDistance;
	PathField _ao_outputPath;

	// Bent normals data
	bool _bn_enabled;
	int _bn_sampleCount;
	float _bn_minDistance;
	float _bn_maxDistance;
	bool _bn_tangentSpace;
	PathField _bn_outputPath;

	// Thickness solver data
	bool _thickness_enabled;
	int _thickness_sampleCount;
	float _thickness_minDistance;
	float _thickness_maxDistance;
	PathField _thickness_outputPath;

	// Errors
	std::string _bakeErrors;
};

FornosUI::FornosUI()
	: _lowPolyNormal(NormalImport::Import)
	, _hiPolyNormal(NormalImport::Import)
	, _bvhTrisPerNode(8)
	, _texWidth(1024)
	, _texHeight(1024)
	, _thickness_enabled(false)
	, _thickness_sampleCount(128)
	, _thickness_minDistance(0.1f)
	, _thickness_maxDistance(1.0f)
	, _normals_enabled(false)
	, _normals_tangentSpace(true)
	, _ao_enabled(false)
	, _ao_sampleCount(128)
	, _ao_minDistance(0.1f)
	, _ao_maxDistance(1.0f)
	, _bn_enabled(false)
	, _bn_sampleCount(128)
	, _bn_minDistance(0.1f)
	, _bn_maxDistance(1.0f)
	, _bn_tangentSpace(true)
{
}

void FornosUI::render()
{
	renderParameters();
	renderWorkInProgress();
	renderErrors();
}

static const char* normalImportNames[3] = { "Import", "Compute per face", "Compute per vertex" };

void FornosUI::renderParameters()
{	
	ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight));
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::Begin(
		"Texture baking", 
		nullptr, 
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove);

	renderParamsShared();
	renderParamsSolverNormals();
	renderParamsSolverAO();
	renderParamsSolverBentNormals();
	renderParamsSolverThickness();

	const bool readyToBake = 
		(_thickness_enabled && !_thickness_outputPath.get().empty()) ||
		(_normals_enabled && !_normals_outputPath.get().empty()) ||
		(_ao_enabled && !_ao_outputPath.get().empty()) ||
		(_bn_enabled && !_bn_outputPath.get().empty());

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

void FornosUI::renderParamsShared()
{
	// Shared data
	{
		parameters_begin();

		// Low mesh
		{
			parameter_openFile("Low Mesh", &_lowPolyPath, "##lo",
				"Low resolution mesh file.\n"
				"Wavefront OBJ files supported.",
				"Select Low-Poly Mesh", ".obj");

			parameter<NormalImport>("Normals", &_lowPolyNormal, normalImportNames, 3, "#lowPolyNormal",
				"How the model normals are imported or computed.");
		}

		// Hi mesh
		{
			parameter_openFile("High Mesh", &_hiPolyPath, "##hi",
				"Optional high resolution mesh file.\n"
				"If not setup it will bake the low resolution mesh.\n"
				"Wavefront OBJ files supported.",
				"Select Hiigh-Poly Mesh", ".obj");

			parameter<NormalImport>("Normals", &_hiPolyNormal, normalImportNames, 3, "#hiPolyNormal",
				"How the model normals are imported or computed.");
		}

		parameter_texSize("Tex Size", &_texWidth, &_texHeight, "#texSize",
			"Texture output size (width x height).\n"
			"Control+click to edit the number.");

		// Advance
		{
			parameter("BVH Tri. Count", &_bvhTrisPerNode, "##BvhTriCount",
				"Maximum number of triangles per BVH leaf node.");
		}

		parameters_end();
	}
}

void FornosUI::renderParamsSolverThickness()
{
	ImGui::PushID("ThicknessSolver");

	bool thicknessExpanded = ImGui::CollapsingHeader("Thickness", ImGuiTreeNodeFlags_AllowItemOverlap);
	ImGui::SameLine(ImGui::GetWindowWidth() - 30);
	ImGui::Checkbox("", &_thickness_enabled);
	if (thicknessExpanded)
	{
		if (!_thickness_enabled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		parameters_begin();

		parameter_saveFile("Output", &_thickness_outputPath, "##thickness",
			"Thickness image output file.",
			"Thickness Map", ".png");

		parameter("Sample count", &_thickness_sampleCount, "##thicknessSampleCount",
			"Number of samples.\nLarger = better & slower.");
		parameter("Min distance", &_thickness_minDistance, "##thicknessMinDistance",
			"Collisions closer than this value are ignored.");
		parameter("Max distance", &_thickness_maxDistance, "##thicknessMaxDistance",
			"Full thickness at this distance.");

		parameters_end();

		if (!_thickness_enabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}

	ImGui::PopID();
}

void FornosUI::renderParamsSolverAO()
{
	ImGui::PushID("AmbientOcclusionSolver");

	bool aoExpanded = ImGui::CollapsingHeader("Ambient Occlusion", ImGuiTreeNodeFlags_AllowItemOverlap);
	ImGui::SameLine(ImGui::GetWindowWidth() - 30);
	ImGui::Checkbox("", &_ao_enabled);
	if (aoExpanded)
	{
		if (!_ao_enabled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		parameters_begin();

		parameter_saveFile("Output", &_ao_outputPath, "##ao",
			"Ambient occlusion image output file.",
			"Ambient Occlusion Map", ".png");

		parameter("Sample count", &_ao_sampleCount, "##aoSampleCount",
			"Number of samples.\nLarger = better & slower.");
		parameter("Min distance", &_ao_minDistance, "##aoMinDistance",
			"Occluders closer than this value are ignored.");
		parameter("Max distance", &_ao_maxDistance, "##aoMaxDistance",
			"Max distance to consider occluders.");

		parameters_end();

		if (!_ao_enabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}

	ImGui::PopID();
}

void FornosUI::renderParamsSolverBentNormals()
{
	ImGui::PushID("BentNormalsSolver");

	bool aoExpanded = ImGui::CollapsingHeader("Bent Normals", ImGuiTreeNodeFlags_AllowItemOverlap);
	ImGui::SameLine(ImGui::GetWindowWidth() - 30);
	ImGui::Checkbox("", &_bn_enabled);
	if (aoExpanded)
	{
		if (!_bn_enabled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		parameters_begin();

		parameter_saveFile("Output", &_bn_outputPath, "##bn",
			"Bent normals image output file.",
			"Bent Normals Map", ".png");

		parameter("Sample count", &_bn_sampleCount, "##bnSampleCount",
			"Number of samples.\nLarger = better & slower.");
		parameter("Min distance", &_bn_minDistance, "##bnMinDistance",
			"Occluders closer than this value are ignored.");
		parameter("Max distance", &_bn_maxDistance, "##bnMaxDistance",
			"Occluders farther than this value are ignored.");
		parameter("Tangent space", &_bn_tangentSpace, "#bnTanSpace",
			"Compute normals in tangent space.");

		parameters_end();

		if (!_bn_enabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}

	ImGui::PopID();
}

void FornosUI::renderParamsSolverNormals()
{
	ImGui::PushID("NormalsSolver");

	bool normalsExpanded = ImGui::CollapsingHeader("Normals", ImGuiTreeNodeFlags_AllowItemOverlap);
	ImGui::SameLine(ImGui::GetWindowWidth() - 30);
	ImGui::Checkbox("", &_normals_enabled);
	if (normalsExpanded)
	{
		if (!_normals_enabled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		parameters_begin();

		parameter_saveFile("Output", &_normals_outputPath, "##normals",
			"Normal map output file.",
			"Normals Map", ".png");

		parameter("Tangent space", &_normals_tangentSpace, "#normalsTanSpace",
			"Computes tangent space normals if this is checked.\n"
			"Otherwise it generates object space normals.");

		parameters_end();

		if (!_normals_enabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}

	ImGui::PopID();
}

void FornosUI::renderWorkInProgress()
{
	if (!tasks.empty())
	{
		if (!ImGui::IsPopupOpen("TaskPopup"))
		{
			ImGui::OpenPopup("TaskPopup");
		}
	}

	ImGui::SetNextWindowSizeConstraints(ImVec2(300, 40), ImVec2(500, 500));
	if (ImGui::BeginPopupModal("TaskPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		if (tasks.empty())
		{
			ImGui::CloseCurrentPopup();
		}
		else
		{
			ImGui::Text("Baking");
			ImGui::SameLine();
			auto task = tasks.back();
			ImGui::Text(task->name());
			ImGui::ProgressBar(task->progress());
		}
		ImGui::EndPopup();
	}
}

void FornosUI::renderErrors()
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

void FornosUI::startBaking()
{
	// TODO: Several of this steps can take long and they will freeze the UI

	std::shared_ptr<Mesh> lowPolyMesh(Mesh::loadWavefrontObj(_lowPolyPath.get().c_str()));
	if (lowPolyMesh)
	{
		switch (_lowPolyNormal)
		{
		case NormalImport::Import: break;
		case NormalImport::ComputePerFace: lowPolyMesh->computeFaceNormals(); break;
		case NormalImport::ComputePerVertex: lowPolyMesh->computeVertexNormals(); break;
		}
	}
	else
	{
		_bakeErrors = "Missing low poly mesh";
		return;
	}

	std::shared_ptr<Mesh> hiPolyMesh = 
		_hiPolyPath.get().empty() ? 
		lowPolyMesh : 
		std::shared_ptr<Mesh>(Mesh::loadWavefrontObj(_hiPolyPath.get().c_str()));
	if (hiPolyMesh && hiPolyMesh != lowPolyMesh)
	{
		switch (_hiPolyNormal)
		{
		case NormalImport::Import: break;
		case NormalImport::ComputePerFace: hiPolyMesh->computeFaceNormals(); break;
		case NormalImport::ComputePerVertex: hiPolyMesh->computeVertexNormals(); break;
		}
	}

	const bool needsTangentSpace = 
		_normals_enabled && _normals_tangentSpace ||
		_bn_enabled && _bn_tangentSpace;

	if (needsTangentSpace)
	{
		lowPolyMesh->computeTangentSpace();
	}

	std::shared_ptr<MapUV> map(MapUV::fromMesh(lowPolyMesh.get(), _texWidth, _texHeight));
	std::shared_ptr<CompressedMapUV> compressedMap(new CompressedMapUV(map.get()));
	if (!map)
	{
		_bakeErrors = "Low poly mesh is missing texture coordinates or normals information";
		return;
	}

	std::shared_ptr<BVH> rootBVH(BVH::createBinary(hiPolyMesh.get(), _bvhTrisPerNode, 8192));

	std::shared_ptr<MeshMapping> meshMapping(new MeshMapping());
	meshMapping->init(compressedMap, hiPolyMesh, rootBVH);

	if (_thickness_enabled)
	{
		ThicknessSolver::Params params;
		params.sampleCount = (uint32_t)_thickness_sampleCount;
		params.minDistance = _thickness_minDistance;
		params.maxDistance = _thickness_maxDistance;
		std::unique_ptr<ThicknessSolver> thicknessSolver(new ThicknessSolver(params));
		thicknessSolver->init(compressedMap, meshMapping);
		tasks.emplace_back(new ThicknessTask(std::move(thicknessSolver), _thickness_outputPath.get().c_str()));
	}

	if (_bn_enabled)
	{
		BentNormalsSolver::Params params;
		params.sampleCount = (uint32_t)_bn_sampleCount;
		params.minDistance = _bn_minDistance;
		params.maxDistance = _bn_maxDistance;
		params.tangentSpace = _bn_tangentSpace;
		std::unique_ptr<BentNormalsSolver> solver(new BentNormalsSolver(params));
		solver->init(compressedMap, meshMapping);
		tasks.emplace_back(new BentNormalsTask(std::move(solver), _bn_outputPath.get().c_str()));
	}


	if (_ao_enabled)
	{
		AmbientOcclusionSolver::Params params;
		params.sampleCount = (uint32_t)_ao_sampleCount;
		params.minDistance = _ao_minDistance;
		params.maxDistance = _ao_maxDistance;
		std::unique_ptr<AmbientOcclusionSolver> solver(new AmbientOcclusionSolver(params));
		solver->init(compressedMap, meshMapping);
		tasks.emplace_back(new AmbientOcclusionTask(std::move(solver), _ao_outputPath.get().c_str()));
	}

	if (_normals_enabled)
	{
		NormalsSolver::Params params;
		params.tangentSpace = _normals_tangentSpace;
		std::unique_ptr<NormalsSolver> normalsSolver(new NormalsSolver(params));
		normalsSolver->init(compressedMap, meshMapping);
		tasks.emplace_back(new NormalsTask(std::move(normalsSolver), _normals_outputPath.get().c_str()));
	}

	tasks.emplace_back(new MeshMappingTask(meshMapping));
}

static void APIENTRY openglCallbackFunction(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam
) 
{
	if (severity == GL_DEBUG_SEVERITY_HIGH)
	{
		fprintf(stderr, "%s\n", message);
	}
}

int main(int argc, char *argv[])
{
	// Setup window
	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) return 1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Fornos: Texture Baking", NULL, NULL);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glfwSwapInterval(1);
	ImGui_ImplGlfwGL3_Init(window, true);
	SetupImGuiStyle(true, 1.0f);

#if 1
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openglCallbackFunction, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);
#endif

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	FornosUI ui;

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		glfwGetWindowSize(window, &windowWidth, &windowHeight);
		glfwPollEvents();

		if (!tasks.empty())
		{
			glfwSwapInterval(0);

			auto task = tasks.back();
			if (task->runStep())
			{
				task->finish(); // Exports map or any other after-compute work
				delete task;
				tasks.pop_back();
			}
		}
		else
		{
			glfwSwapInterval(1);
		}

		ImGui_ImplGlfwGL3_NewFrame();

#if 0
		// For UI reference
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
		ImGui::ShowDemoWindow();
#endif
		
		ui.render();

		// Rendering
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplGlfwGL3_Shutdown();
	glfwTerminate();

	return 0;
}
