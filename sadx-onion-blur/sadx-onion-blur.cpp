#include "stdafx.h"
#include "IniFile.hpp"

// Tails always renders two additional tail models, so we can
// use this to alternate between an alpha of .75 and .5
// Same for Knuckles because he also renders two models.

static bool tails_hack = false;

static void __cdecl njAction_Onion(NJS_ACTION* action, float frame)
{
	const auto frame_count = static_cast<float>(action->motion->nbFrame);

	const NJS_ARGB color_orig = cur_argb;
	const uint32_t control_3d_orig = nj_control_3d_flag_;
	const uint32_t constant_attr_orig = _nj_constant_attr_or_;

	nj_control_3d_flag_ |= NJD_CONTROL_3D_CONSTANT_ATTR | NJD_CONTROL_3D_CONSTANT_MATERIAL;

	OnConstantAttr(0, NJD_FLAG_USE_ALPHA);
	njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
	njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);

	float alpha = 0.75f;

	for (int i = 0; i < 2; ++i)
	{
		SetMaterial(alpha, 1.0f, 1.0f, 1.0f);

		alpha -= 0.25f;
		frame -= 2.0f;

		if (frame < 0.0f)
		{
			frame = frame_count + frame;
		}

		late_Action(action, frame, LATE_WZ);
	}

	_nj_constant_attr_or_ = constant_attr_orig;
	nj_control_3d_flag_ = control_3d_orig;
	cur_argb = color_orig;
}

static const void* DrawSonicMotion_loc = reinterpret_cast<const void*>(0x00494400); // DrawSonicMotion usercall

static void __cdecl DrawSonicMotion_o(int a1, playerwk* a2)
{
	__asm
	{
		mov eax, a1
		mov esi, a2
		call DrawSonicMotion_loc
	}
}

static void __cdecl DrawSonicMotion_c(int anim_index, playerwk* data2)
{
	if (!gu8flgPlayingMetalSonic && (data2->flag & 1) != 0) // Not in the air
	{
		switch (anim_index)
		{
			case 13: // Running
			case 14: // Spindash
				ds_ActionClip(data2->mj.plactptr[anim_index].actptr, data2->mj.nframe, 0.0f);
				njAction_Onion(data2->mj.plactptr[anim_index].actptr, data2->mj.nframe);
				return;
			default:
				break;
		}
	}

	DrawSonicMotion_o(anim_index, data2);
}

static void __declspec(naked) DrawSonicMotion_asm()
{
	__asm
	{
		push esi // a2
		push eax // a1

		call DrawSonicMotion_c

		pop eax // a1
		pop esi // a2
		retn
	}
}

static void __cdecl njAction_TailsKnucklesWrapper(NJS_ACTION* action, float frame)
{
	const uint32_t control_3d_orig = nj_control_3d_flag_;
	const uint32_t constant_attr_orig = _nj_constant_attr_or_;
	const NJS_ARGB color_orig = cur_argb;

	nj_control_3d_flag_ |= NJD_CONTROL_3D_CONSTANT_ATTR | NJD_CONTROL_3D_CONSTANT_MATERIAL;

	OnConstantAttr(0, NJD_FLAG_USE_ALPHA);
	njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
	njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);

	float alpha = tails_hack ? 0.5f : 0.75f;
	tails_hack = !tails_hack;
	SetMaterial(alpha, 1.0f, 1.0f, 1.0f);

	const auto last_bias = late_z_ofs___;
	late_z_ofs___ = -47952.0f;

	___dsSetPalette(2);
	late_Action(action, frame, LATE_WZ);
	___dsSetPalette(0);

	late_z_ofs___ = last_bias;

	_nj_constant_attr_or_ = constant_attr_orig;
	nj_control_3d_flag_ = control_3d_orig;
	cur_argb = color_orig;
}

extern "C"
{
	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };

	__declspec(dllexport) void __cdecl Init(const char* path, const HelperFunctions& helperFunctions)
	{
		const std::string s_path(path);
		const std::string s_config_ini(s_path + "\\config.ini");
		const IniFile* const config = new IniFile(s_config_ini);
		// Sonic's motion blur
		if (config->getBool("General", "EnableSonic", true))
		{
			WriteCall(reinterpret_cast<void*>(0x004947B7), &DrawSonicMotion_asm);
			WriteCall(reinterpret_cast<void*>(0x00494B00), &DrawSonicMotion_asm);
		}
		// Tails' tails
		if (config->getBool("General", "EnableTails", true))
		{
			WriteCall(reinterpret_cast<void*>(0x00461156), njAction_TailsKnucklesWrapper);
			WriteData<5>(reinterpret_cast<void*>(0x004610AF), 0x90i8);
			WriteData<5>(reinterpret_cast<void*>(0x004611A9), 0x90i8);
			// Allows tail onion skin to be rendered while paused
			WriteData<2>(reinterpret_cast<void*>(0x0046110E), 0x90i8);
		}
		// Knuckles' motion blur
		if (config->getBool("General", "EnableKnuckles", true))
		{
			WriteCall(reinterpret_cast<void*>(0x00472626), njAction_TailsKnucklesWrapper);
		}
	}
}
