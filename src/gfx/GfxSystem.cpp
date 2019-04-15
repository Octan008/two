//  Copyright (c) 2019 Hugo Amiard hugo.amiard@laposte.net
//  This software is provided 'as-is' under the zlib License, see the LICENSE.txt file.
//  This notice and the license may not be removed or altered from any source distribution.

#include <gfx/Cpp20.h>

#include <bx/timer.h>
#include <bx/file.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#ifdef MUD_MODULES
module mud.gfx;
#else
#include <stl/array.h>
#include <stl/string.h>
#include <stl/map.h>
#include <pool/ObjectPool.hpp>
#include <infra/ToString.h>
#include <infra/File.h>
#include <math/Image256.h>
#include <geom/Geom.hpp>
#include <geom/Geometry.h>
#include <gfx/Types.h>
#include <gfx/GfxSystem.h>
#include <gfx/Material.h>
#include <gfx/Program.h>
#include <gfx/Draw.h>
#include <gfx/Viewport.h>
#include <gfx/RenderTarget.h>
#include <gfx/Scene.h>
#include <gfx/Texture.h>
#include <gfx/Model.h>
#include <gfx/Prefab.h>
#include <gfx/Particles.h>
#include <gfx/Assets.h>
#include <gfx/Importer.h>
#include <gfx/Pipeline.h>
#include <gfx/Filter.h>
#include <gfx/Skeleton.h>
#endif

#include <Tracy.hpp>

//#define MUD_GFX_THREADED
#define POLL_AT_END 0

namespace mud
{
	struct GfxSystem::Impl
	{
		vector<string> m_resource_paths;

		vector<GfxWindow*> m_contexts;
		vector<Scene*> m_scenes;

		bx::FileReader m_file_reader;
		bx::FileWriter m_file_writer;

		unique<TPool<Mesh>> m_meshes;
		unique<TPool<Rig>> m_rigs;
		unique<TPool<Animation>> m_animations;

		unique<AssetStore<Texture>> m_textures;
		unique<AssetStore<Program>> m_programs;
		unique<AssetStore<Material>> m_materials;
		unique<AssetStore<Model>> m_models;
		unique<AssetStore<Flow>> m_particles;
		unique<AssetStore<Prefab>> m_prefabs;

		table<ModelFormat, Importer*> m_importers;
		table<Shading, RenderFunc> m_renderers;

		Texture* m_white_texture = nullptr;
		Texture* m_black_texture = nullptr;
		Texture* m_normal_texture = nullptr;

		SymbolIndex m_symbols;

#ifdef MUD_GFX_THREADED
		vector<bgfx::Encoder*> m_encoders;
#endif
	};

	GfxWindow::GfxWindow(GfxSystem& gfx, const string& name, const uvec2& size, bool fullscreen, bool main)
		: BgfxContext(gfx, name, size, fullscreen, main, false)
		, m_gfx(gfx)
		, m_target()
	{
		if(!gfx.m_initialized)
			gfx.init(*this);
		m_target = oconstruct<RenderTarget>(size, main ? nullptr : m_native_handle);
		gfx.m_impl->m_contexts.push_back(this);
	}

	GfxWindow::~GfxWindow()
	{}

	void GfxWindow::reset_fb(const uvec2& size)
	{
		bgfx::reset(uint16_t(size.x), uint16_t(size.y), BGFX_RESET_NONE);
		if(size.x == 0 || size.y == 0)
			m_target = nullptr;
		else
		{
			if(!m_target || size != m_target->m_size)
				m_target = oconstruct<RenderTarget>(size);
		}
		m_vg_handle = m_reset_vg(*this, *m_vg);
	}

	GfxSystem::GfxSystem(const string& resource_path)
		: BgfxSystem(resource_path)
		, m_impl(make_unique<Impl>())
		, m_renderer(*this)
	{
		Program::ms_gfx = this;
		Material::ms_gfx = this;
		Model::ms_gfx = this;

		this->add_resource_path(resource_path, false);
	}

	GfxSystem::~GfxSystem()
	{}

	bx::FileReaderI& GfxSystem::file_reader() { return m_impl->m_file_reader; }
	bx::FileWriterI& GfxSystem::file_writer() { return m_impl->m_file_writer; }

	TPool<Mesh>& GfxSystem::meshes() { return *m_impl->m_meshes; }
	TPool<Rig>& GfxSystem::rigs() { return *m_impl->m_rigs; }
	TPool<Animation>& GfxSystem::animations() { return *m_impl->m_animations; }

	AssetStore<Texture>& GfxSystem::textures() { return *m_impl->m_textures; }
	AssetStore<Program>& GfxSystem::programs() { return *m_impl->m_programs; }
	AssetStore<Material>& GfxSystem::materials() { return *m_impl->m_materials; }
	AssetStore<Model>& GfxSystem::models() { return *m_impl->m_models; }
	AssetStore<Flow>& GfxSystem::flows() { return *m_impl->m_particles; }
	AssetStore<Prefab>& GfxSystem::prefabs() { return *m_impl->m_prefabs; }

	void GfxSystem::add_importer(ModelFormat format, Importer& importer)
	{
		m_impl->m_importers[format] = &importer;
	}

	Importer* GfxSystem::importer(ModelFormat format)
	{
		return m_impl->m_importers[format];
	}

	void GfxSystem::init(GfxWindow& context)
	{
		BgfxSystem::init(context);

		m_flip_y = bgfx::getCaps()->originBottomLeft;

		m_impl->m_meshes = make_unique<TPool<Mesh>>();
		m_impl->m_rigs = make_unique<TPool<Rig>>();
		m_impl->m_animations = make_unique<TPool<Animation>>();
		
		auto load_tex = [&](Texture& texture, const string& path, const NoConfig& config) { load_texture(*this, texture, path); };

		m_impl->m_textures = make_unique<AssetStore<Texture>>(*this, "textures/", load_tex);
		m_impl->m_programs = make_unique<AssetStore<Program>>(*this, "programs/", ".prg");
		m_impl->m_materials = make_unique<AssetStore<Material>>(*this, "materials/", ".mtl");
		m_impl->m_models = make_unique<AssetStore<Model>>(*this, "models/");
		m_impl->m_particles = make_unique<AssetStore<Flow>>(*this, "particles/", ".ptc");
		//m_impl->m_prefabs = make_unique<AssetStore<Prefab>>(*this, "prefabs/", ".pfb");
		m_impl->m_prefabs = make_unique<AssetStore<Prefab>>(*this, "models/");

		m_impl->m_white_texture = this->textures().file("white.png");
		m_impl->m_black_texture = this->textures().file("black.png");
		m_impl->m_normal_texture = this->textures().file("normal.png");

		// create point mesh
		{
			MeshPacker geometry;

			geometry.m_primitive = PrimitiveType::Triangles;

			geometry.m_positions = { vec3(-1, 1, 0), vec3(-1, -1, 0), vec3(1, -1, 0), vec3(1, 1, 0) };
			geometry.m_uv0s = { vec2(0, 1), vec2(0, 0), vec2(1, 0), vec2(1, 1) };
			geometry.m_indices = { 0, 1, 2, 2, 3, 0 };

			this->create_model_geo("point", geometry);
		}

		// create fat line mesh
		{
			MeshPacker geometry;

			geometry.m_positions = { vec3(-1, 2, 0), vec3(1, 2, 0), vec3(-1, 1, 0), vec3(1, 1, 0), vec3(-1, 0, 0), vec3(1, 0, 0), vec3(-1, -1, 0), vec3(1, -1, 0) };
			geometry.m_uv0s = { vec2(-1, 2), vec2(1, 2), vec2(-1, 1), vec2(1, 1), vec2(-1, -1), vec2(1, -1), vec2(-1, -2), vec2(1, -2) };
			geometry.m_indices = { 0, 2, 1, 2, 3, 1, 2, 4, 3, 4, 5, 3, 4, 6, 5, 6, 7, 5 };

			this->create_model_geo("line", geometry);
		}
	}

	void GfxSystem::default_pipeline()
	{
		this->init_pipeline(pipeline_minimal);
	}

	void GfxSystem::init_pipeline(PipelineDecl decl)
	{
#ifdef MUD_GFX_DEFERRED
		decl(*this, m_renderer, true);
#else
		decl(*this, m_renderer, false);
#endif

		for(auto& block : m_renderer.m_gfx_blocks)
			block->init_block();

		this->set_renderer(Shading::Solid, render_solid);
		this->set_renderer(Shading::Clear, render_clear);

		this->create_debug_materials();
	}

	void GfxSystem::add_resource_path(const string& path, bool relative)
	{
		printf("[info] gfx - resource path: %s\n", path.c_str());
		m_impl->m_resource_paths.push_back(relative ? m_resource_path + "/" + path : path);
	}

	void GfxSystem::set_renderer(Shading shading, const RenderFunc& renderer)
	{
		m_impl->m_renderers[shading] = renderer;
	}

	RenderFunc GfxSystem::renderer(Shading shading)
	{
		return *m_impl->m_renderers[shading];
	}

	GfxWindow& GfxSystem::context(size_t index)
	{
		return *m_impl->m_contexts[index];
	}

	RenderTarget& GfxSystem::main_target()
	{
		return *this->context(0).m_target;
	}

	bool GfxSystem::begin_frame()
	{
		m_render_frame = { m_frame, m_time, m_delta_time, Render::s_render_pass_id };

		{
			ZoneScopedNC("programs", tracy::Color::Cyan);

			for(Program* program : m_impl->m_programs->m_vector)
				program->update(*this);
		}

		{
			ZoneScopedNC("renderers", tracy::Color::Cyan);

			for(auto& block : m_renderer.m_gfx_blocks)
				block->begin_frame(m_render_frame);
		}

		bool pursue = true;
		{
			ZoneScopedNC("gfx contexts begin", tracy::Color::Cyan);
		
			for(GfxWindow* context : m_impl->m_contexts)
				pursue &= context->begin_frame();
		}

#ifdef MUD_GFX_THREADED
		{
			ZoneScopedNC("gfx begin", tracy::Color::Cyan);

			m_num_encoders = min(uint32_t(4U), bgfx::getCaps()->limits.maxEncoders);

			m_encoders[0] = bgfx::begin();
			for(size_t i = 1; i < m_num_encoders; ++i)
				m_encoders[i] = bgfx::begin(true);
		}
#endif

		return pursue;
	}

	void GfxSystem::render_contexts()
	{
		for(GfxWindow* context : m_impl->m_contexts)
			for(Viewport* viewport : context->m_viewports)
				if(viewport->m_autorender)
				{
					ZoneScopedNC("gfx viewport", tracy::Color::Cyan);

					RenderFunc renderer = this->renderer(viewport->m_shading);
					this->render(viewport->m_shading, renderer, *context->m_target, *viewport);
				}
	}

	void GfxSystem::end_frame()
	{
		{
			ZoneScopedNC("gfx contexts render", tracy::Color::Cyan);

			for(GfxWindow* context : m_impl->m_contexts)
				context->render_frame();
		}

#ifdef MUD_GFX_THREADED
		{
			ZoneScopedNC("gfx end", tracy::Color::Cyan);

			for(size_t i = 1; i < m_num_encoders; ++i)
				bgfx::end(m_encoders[i]);
		}
#endif

		{
			ZoneScopedNC("gfx contexts end", tracy::Color::Cyan);
		
			for(GfxWindow* context : m_impl->m_contexts)
				context->end_frame();
		}

		for(Program* program : m_impl->m_programs->m_vector)
			program->update(*this);

		{
			ZoneScopedNC("gfx frame", tracy::Color::Cyan);
			BgfxSystem::end_frame();
		}

		for(Program* program : m_impl->m_programs->m_vector)
			program->update(*this);
	}

	void GfxSystem::render(Shading shading, RenderFunc renderer, RenderTarget& target, Viewport& viewport)
	{
		Render render = { shading, viewport, target, m_render_frame };
		m_renderer.submit(render, renderer);
	}

	RenderFrame GfxSystem::render_frame()
	{
		return { m_frame, m_time, m_delta_time, Render::s_render_pass_id };
	}

	LocatedFile GfxSystem::locate_file(const string& file, span<string> extensions)
	{
		for(const string& path : m_impl->m_resource_paths)
			for(size_t i = 0; i < extensions.size(); ++i)
			{
				string filepath = path + "/" + file + extensions[i];
				if(file_exists(filepath))
				{
					return { path, file, extensions[i], i };
				}
			}
		return {};
	}

	LocatedFile GfxSystem::locate_file(const string& file)
	{
		return this->locate_file(file, { "" });
	}

	Texture& GfxSystem::default_texture(TextureHint hint)
	{
		if(hint == TextureHint::Black)
			return *m_impl->m_black_texture;
		else if(hint == TextureHint::White)
			return *m_impl->m_white_texture;
		else if(hint == TextureHint::Normal || true)
			return *m_impl->m_normal_texture;
	}

	void GfxSystem::create_debug_materials()
	{
		Material& debug = this->fetch_material("debug", "solid");
		UNUSED(debug);

		Material& alpha = this->fetch_material("debug_alpha", "solid");
		alpha.m_solid.m_colour = Colour(0.2f, 0.1f);

		Material& pbr = this->fetch_material("debug_pbr", "pbr/pbr");
		UNUSED(pbr);
	}

	Model& GfxSystem::create_model(const string& name)
	{
		Model& model = this->models().create(name);
		Mesh& mesh = model.add_mesh(name);

		model.add_item(mesh, bxidentity());
		return model;
	}

	Model& GfxSystem::create_model_geo(const string& name, const MeshPacker& geometry, bool readback, bool optimize)
	{
		Model& model = this->models().create(name);
		Mesh& mesh = model.add_mesh(name, readback);

		mesh.write(geometry, optimize);

		model.add_item(mesh, bxidentity());
		model.prepare();
		return model;
	}

	Model& GfxSystem::create_model_gpu(const string& name, const GpuMesh& gpu_mesh, bool readback, bool optimize)
	{
		Model& model = this->models().create(name);
		Mesh& mesh = model.add_mesh(name, readback);

		mesh.upload(gpu_mesh, optimize);

		model.add_item(mesh, bxidentity());
		model.prepare();
		return model;
	}

	Material& GfxSystem::debug_material()
	{
		return *this->materials().get("debug_pbr");
	}

	Material& GfxSystem::fetch_material(const string& name, const string& shader, bool builtin)
	{
		Program* program = this->programs().file(shader);
		Material& material = this->materials().fetch(name);
		material.m_builtin = builtin;
		material.m_program = program;
		return material;
	}

	Material& GfxSystem::fetch_image256_material(const Image256& image)
	{
		string name = "Image256_" + to_string((uintptr_t)&image);
		Material* material = this->materials().get(name);

		if(!material)
		{
			string image_name = "Image256_" + to_string((uintptr_t)&image);
			auto initializer = [&](Texture& texture) { auto data = image.read32(); texture.load_rgba(image.m_size, data); };

			Texture& texture = this->textures().fetch(image_name);
			initializer(texture);
			material = &this->fetch_material(name, "solid");
			material->m_solid.m_colour = &texture;
		}

		return *material;
	}

	Model& GfxSystem::shape(const Shape& shape, const Symbol& symbol, DrawMode draw_mode)
	{
		return m_impl->m_symbols.symbol_model(symbol, shape, draw_mode);
	}

	Material& GfxSystem::symbol_material(const Symbol& symbol, DrawMode draw_mode)
	{
		if(symbol.m_image256)
			return this->fetch_image256_material(*symbol.m_image256);
		else
			return m_impl->m_symbols.symbol_material(*this, symbol, draw_mode);
	}

namespace gfx
{
	Model& model_suzanne(GfxSystem& gfx)
	{
		constexpr double position[] = { 0.46875,0.242188,0.757812,0.5,0.09375,0.6875,0.5625,0.242188,0.671875,-0.5,0.09375,0.6875,-0.46875,0.242188,0.757812,-0.5625,0.242188,0.671875,0.546875,0.0546875,0.578125,0.625,0.242188,0.5625,-0.546875,0.0546875,0.578125,-0.625,0.242188,0.5625,0.351562,-0.0234375,0.617188,-0.351562,-0.0234375,0.617188,0.4375,0.164063,0.765625,0.351562,0.03125,0.71875,-0.351562,0.03125,0.71875,-0.4375,0.164063,0.765625,0.351562,0.132813,0.78125,0.203125,0.09375,0.742188,-0.203125,0.09375,0.742188,-0.351562,0.132813,0.78125,0.15625,0.0546875,0.648438,-0.15625,0.0546875,0.648438,0.140625,0.242188,0.742188,-0.140625,0.242188,0.742188,-0.078125,0.242188,0.65625,0.242188,0.242188,0.796875,0.273438,0.164063,0.796875,-0.242188,0.242188,0.796875,0.203125,0.390625,0.742188,-0.203125,0.390625,0.742188,0.078125,0.242188,0.65625,-0.15625,0.4375,0.648438,0.351562,0.453125,0.71875,0.15625,0.4375,0.648438,-0.351562,0.453125,0.71875,-0.351562,0.515625,0.617188,0.351562,0.359375,0.78125,0.273438,0.328125,0.796875,-0.351562,0.359375,0.78125,0.4375,0.328125,0.765625,-0.4375,0.328125,0.765625,-0.5,0.390625,0.6875,0.5,0.390625,0.6875,0.351562,0.515625,0.617188,-0.546875,0.4375,0.578125,0.546875,0.4375,0.578125,0.476562,0.242188,0.773438,-0.476562,0.242188,0.773438,-0.445312,0.335938,0.78125,0.445312,0.335938,0.78125,-0.351562,0.375,0.804688,0.351562,0.375,0.804688,-0.273438,0.328125,0.796875,-0.265625,0.335938,0.820312,0.265625,0.335938,0.820312,-0.226562,0.242188,0.820312,0.265625,0.15625,0.820312,0.226562,0.242188,0.820312,-0.265625,0.15625,0.820312,0.351562,0.117188,0.804688,-0.351562,0.117188,0.804688,-0.273438,0.164063,0.796875,0.445312,0.15625,0.78125,-0.445312,0.15625,0.78125,0.351562,0.242188,0.828125,-0.351562,0.242188,0.828125,0.164062,-0.929688,0.632813,0,-0.984375,0.578125,0.179688,-0.96875,0.554688,-0.164062,-0.929688,0.632813,0,-0.945312,0.640625,0.234375,-0.914062,0.632813,0.328125,-0.945312,0.523438,-0.234375,-0.914062,0.632813,-0.179688,-0.96875,0.554688,0.367188,-0.890625,0.53125,-0.367188,-0.890625,0.53125,-0.328125,-0.945312,0.523438,0.351562,-0.695312,0.570313,0.265625,-0.820312,0.664063,-0.265625,-0.820312,0.664063,-0.351562,-0.695312,0.570313,0.3125,-0.4375,0.570312,0.25,-0.703125,0.6875,-0.25,-0.703125,0.6875,-0.3125,-0.4375,0.570312,0.203125,-0.1875,0.5625,0.398438,-0.046875,0.671875,0.125,-0.101562,0.8125,-0.398438,-0.046875,0.671875,-0.203125,-0.1875,0.5625,-0.125,-0.101562,0.8125,0.632812,-0.0390625,0.539062,0.4375,-0.140625,0.53125,-0.632812,-0.0390625,0.539062,-0.617188,0.0546875,0.625,0.726562,0.203125,0.601562,0.617188,0.0546875,0.625,-0.726562,0.203125,0.601562,0.859375,0.429688,0.59375,0.828125,0.148438,0.445312,-0.859375,0.429688,0.59375,-0.742188,0.375,0.65625,0.710938,0.484375,0.625,0.742188,0.375,0.65625,-0.710938,0.484375,0.625,-0.6875,0.414063,0.726562,0.492188,0.601563,0.6875,0.6875,0.414063,0.726562,-0.492188,0.601563,0.6875,-0.4375,0.546875,0.796875,0.3125,0.640625,0.835938,0.4375,0.546875,0.796875,-0.3125,0.640625,0.835938,0.15625,0.71875,0.757812,0.320312,0.757813,0.734375,-0.15625,0.71875,0.757812,-0.203125,0.617188,0.851562,0.0625,0.492188,0.75,0.203125,0.617188,0.851562,-0.0625,0.492188,0.75,-0.101562,0.429688,0.84375,0,0.429688,0.742188,0.101562,0.429688,0.84375,0,0.351563,0.820312,0.25,0.46875,0.757812,0.164062,0.414063,0.773438,-0.25,0.46875,0.757812,0.328125,0.476563,0.742188,0.429688,0.4375,0.71875,-0.328125,0.476563,0.742188,0.601562,0.375,0.664062,-0.429688,0.4375,0.71875,0.640625,0.296875,0.648438,-0.601562,0.375,0.664062,0.625,0.1875,0.648438,-0.640625,0.296875,0.648438,0.492188,0.0625,0.671875,-0.625,0.1875,0.648438,0.375,0.015625,0.703125,-0.492188,0.0625,0.671875,-0.375,0.015625,0.703125,0,0.046875,0.726562,0.125,0.304688,0.765625,-0.125,0.304688,0.765625,0,0.210938,0.765625,0.132812,0.210938,0.757812,-0.132812,0.210938,0.757812,0.164062,0.140625,0.75,-0.164062,0.140625,0.75,0.0625,-0.882812,0.695313,-0.0625,-0.882812,0.695313,0.117188,-0.835937,0.710938,-0.117188,-0.835937,0.710938,0.109375,-0.71875,0.734375,0.210938,-0.445312,0.710938,0.117188,-0.6875,0.734375,-0.117188,-0.6875,0.734375,-0.210938,-0.445312,0.710938,-0.109375,-0.71875,0.734375,0,-0.328125,0.742188,0.078125,-0.445312,0.75,0.0859375,-0.289062,0.742188,-0.078125,-0.445312,0.75,0,-0.445312,0.75,0,-0.679687,0.734375,0,-0.765625,0.734375,0.125,-0.226562,0.75,0.09375,-0.273437,0.78125,-0.09375,-0.273437,0.78125,-0.125,-0.226562,0.75,-0.0859375,-0.289062,0.742188,0.109375,-0.132812,0.78125,0.101562,-0.148437,0.742188,-0.109375,-0.132812,0.78125,-0.132812,-0.226562,0.796875,0.0390625,-0.125,0.78125,0,-0.140625,0.742188,-0.0390625,-0.125,0.78125,-0.101562,-0.148437,0.742188,0,-0.1875,0.796875,0,-0.195312,0.75,0,-0.320312,0.78125,0,-0.289062,0.804688,-0.078125,-0.25,0.804688,0.046875,-0.148437,0.8125,-0.046875,-0.148437,0.8125,0.09375,-0.15625,0.8125,-0.09375,-0.15625,0.8125,0.132812,-0.226562,0.796875,-0.109375,-0.226562,0.828125,0.078125,-0.25,0.804688,0.109375,-0.226562,0.828125,0,-0.203125,0.828125,0.164062,-0.242187,0.710938,-0.164062,-0.242187,0.710938,-0.179688,-0.3125,0.710938,0.179688,-0.3125,0.710938,0.257812,-0.3125,0.554688,-0.257812,-0.3125,0.554688,0.234375,-0.25,0.554688,-0.234375,-0.25,0.554688,0.09375,-0.742187,0.726563,-0.09375,-0.742187,0.726563,0,-0.773437,0.71875,0.09375,-0.820312,0.710938,-0.09375,-0.820312,0.710938,0.046875,-0.867187,0.6875,-0.046875,-0.867187,0.6875,0,-0.890625,0.6875,0,-0.875,0.6875,0,-0.859375,0.632813,-0.046875,-0.851562,0.632813,0.09375,-0.8125,0.640625,0.046875,-0.851562,0.632813,-0.09375,-0.8125,0.640625,0.09375,-0.75,0.664063,-0.09375,-0.75,0.664063,0,-0.78125,0.65625,0.1875,0.15625,0.773438,0.171875,0.21875,0.78125,-0.1875,0.15625,0.773438,-0.171875,0.21875,0.78125,0.179688,0.296875,0.78125,-0.179688,0.296875,0.78125,0.210938,0.375,0.78125,-0.210938,0.375,0.78125,-0.226562,0.109375,0.78125,0.375,0.0625,0.742188,0.226562,0.109375,0.78125,-0.375,0.0625,0.742188,0.476562,0.101563,0.71875,-0.476562,0.101563,0.71875,0.578125,0.195313,0.679688,-0.578125,0.195313,0.679688,0.585938,0.289063,0.6875,-0.585938,0.289063,0.6875,-0.5625,0.351563,0.695312,0.5625,0.351563,0.695312,-0.421875,0.398438,0.773438,0.335938,0.429688,0.757812,0.421875,0.398438,0.773438,-0.335938,0.429688,0.757812,0.273438,0.421875,0.773438,-0.273438,0.421875,0.773438,0.234375,0.359375,0.757812,0.28125,0.398438,0.765625,-0.234375,0.359375,0.757812,-0.28125,0.398438,0.765625,0.335938,0.40625,0.75,-0.335938,0.40625,0.75,0.414062,0.390625,0.75,-0.414062,0.390625,0.75,0.53125,0.335938,0.679688,-0.53125,0.335938,0.679688,0.554688,0.28125,0.671875,-0.554688,0.28125,0.671875,0.546875,0.210938,0.671875,-0.546875,0.210938,0.671875,0.460938,0.117188,0.703125,-0.460938,0.117188,0.703125,0.375,0.0859375,0.726562,-0.375,0.0859375,0.726562,0.242188,0.125,0.757812,-0.242188,0.125,0.757812,0.203125,0.171875,0.75,-0.203125,0.171875,0.75,0.195312,0.296875,0.757812,-0.195312,0.296875,0.757812,0.195312,0.226563,0.75,-0.195312,0.226563,0.75,0.109375,0.460938,0.609375,0,0.40625,0.601562,-0.109375,0.460938,0.609375,0.195312,0.664062,0.617188,-0.195312,0.664062,0.617188,-0.320312,0.757813,0.734375,-0.335938,0.6875,0.59375,0.335938,0.6875,0.59375,-0.484375,0.554688,0.554688,0.484375,0.554688,0.554688,-0.679688,0.453125,0.492187,0.796875,0.40625,0.460937,0.679688,0.453125,0.492187,-0.796875,0.40625,0.460937,-0.828125,0.148438,0.445312,-0.773438,0.164063,0.375,0.601562,1.80992e-08,0.414062,0.773438,0.164063,0.375,-0.601562,1.80992e-08,0.414062,0.4375,-0.09375,0.46875,-0.4375,-0.09375,0.46875,0,-0.484375,0.28125,0.125,-0.539062,0.359375,0,-0.570312,0.320313,-0.125,-0.539062,0.359375,-0.179688,-0.414062,0.257813,0.140625,-0.757812,0.367188,0,-0.804688,0.34375,-0.140625,-0.757812,0.367188,0.164062,-0.945312,0.4375,0,-0.976562,0.460938,-0.164062,-0.945312,0.4375,0.328125,-0.914062,0.398438,-0.328125,-0.914062,0.398438,0.289062,-0.710938,0.382813,-0.289062,-0.710938,0.382813,0.25,-0.5,0.390625,-0.25,-0.5,0.390625,0.179688,-0.414062,0.257813,0.234375,-0.351562,0.40625,-0.234375,-0.351562,0.40625,0.21875,-0.28125,0.429688,-0.21875,-0.28125,0.429688,-0.210938,-0.226562,0.46875,0.203125,-0.171875,0.5,-0.203125,-0.171875,0.5,-0.4375,-0.140625,0.53125,0.335938,0.0546875,-0.664062,0,-0.195313,-0.671875,0,0.0703125,-0.828125,-0.335938,0.0546875,-0.664062,-0.34375,-0.148438,-0.539062,0.34375,-0.148438,-0.539062,0,-0.382813,-0.351562,-0.296875,-0.3125,-0.265625,0.210938,-0.390625,0.164063,0,-0.460938,0.1875,-0.210938,-0.390625,0.164063,0.734375,-0.046875,0.0703125,0.851562,0.234375,0.0546875,-0.734375,-0.046875,0.0703125,-0.851562,0.234375,0.0546875,0.460938,0.4375,-0.703125,0,0.5625,-0.851562,-0.460938,0.4375,-0.703125,0.453125,0.851562,0.234375,0,0.984375,-0.078125,0,0.898438,0.289062,-0.453125,0.851562,0.234375,-0.453125,0.929688,-0.0703125,0.453125,0.867188,-0.382813,0,0.898438,-0.546875,-0.453125,0.867188,-0.382813,0.726562,0.40625,0.335937,0.632812,0.453125,0.28125,-0.726562,0.40625,0.335937,-0.632812,0.453125,0.28125,0.796875,0.5625,0.125,0.640625,0.703125,0.0546875,-0.796875,0.5625,0.125,-0.640625,0.703125,0.0546875,0.796875,0.617188,-0.117188,0.640625,0.75,-0.195313,-0.796875,0.617188,-0.117188,-0.640625,0.75,-0.195313,0.796875,0.539062,-0.359375,0.640625,0.679688,-0.445313,-0.796875,0.539062,-0.359375,-0.640625,0.679688,-0.445313,0.617188,0.328125,-0.585938,0.773438,0.265625,-0.4375,-0.617188,0.328125,-0.585938,0.453125,0.929688,-0.0703125,0.460938,0.523438,0.429687,-0.460938,0.523438,0.429687,0,0.570312,0.570312,0.859375,0.320312,-0.046875,-0.859375,0.320312,-0.046875,0.820312,0.328125,-0.203125,-0.820312,0.328125,-0.203125,0.296875,-0.3125,-0.265625,0.40625,-0.171875,0.148438,-0.40625,-0.171875,0.148438,-0.429688,-0.195313,-0.210937,0.59375,-0.125,-0.164062,-0.59375,-0.125,-0.164062,0.210938,-0.226562,0.46875,0.640625,-0.00781252,-0.429688,-0.640625,-0.00781252,-0.429688,-0.484375,0.0234375,-0.546875,0.429688,-0.195313,-0.210937,0.484375,0.0234375,-0.546875,0.890625,0.40625,-0.234375,1.01562,0.414062,-0.289063,1.02344,0.476562,-0.3125,-0.890625,0.40625,-0.234375,-1.01562,0.414062,-0.289063,-0.921875,0.359375,-0.21875,1.1875,0.4375,-0.390625,1.23438,0.507812,-0.421875,-1.1875,0.4375,-0.390625,-1.02344,0.476562,-0.3125,-1.23438,0.507812,-0.421875,1.35156,0.320312,-0.421875,-1.35156,0.320312,-0.421875,-1.26562,0.289062,-0.40625,1.26562,0.289062,-0.40625,1.28125,0.0546875,-0.429688,-1.28125,0.0546875,-0.429688,-1.21094,0.078125,-0.40625,1.21094,0.078125,-0.40625,1.03906,-0.101563,-0.328125,-1.03906,-0.101563,-0.328125,-1.03125,-0.0390625,-0.304688,0.828125,-0.0703125,-0.132812,0.773438,-0.140625,-0.125,-0.828125,-0.0703125,-0.132812,-0.773438,-0.140625,-0.125,1.03125,-0.0390625,-0.304688,0.882812,-0.0234375,-0.210938,-0.882812,-0.0234375,-0.210938,1.03906,-1.60503e-08,-0.367188,-1.03906,-1.60503e-08,-0.367188,1.23438,0.25,-0.445312,-1.23438,0.25,-0.445312,-1.1875,0.09375,-0.445312,1.17188,0.359375,-0.4375,-1.17188,0.359375,-0.4375,1.02344,0.34375,-0.359375,-1.02344,0.34375,-0.359375,0.945312,0.304688,-0.289062,-0.945312,0.304688,-0.289062,0.726562,-3.07346e-09,-0.0703125,-0.726562,-3.07346e-09,-0.0703125,-0.71875,-0.0234375,-0.171875,0.71875,-0.0234375,-0.171875,0.921875,0.359375,-0.21875,0.8125,-0.015625,-0.273438,-0.8125,-0.015625,-0.273438,0.71875,0.0390625,-0.1875,0.84375,0.015625,-0.273438,-0.71875,0.0390625,-0.1875,0.757812,0.09375,-0.273438,0.820312,0.0859375,-0.273438,-0.84375,0.015625,-0.273438,-0.757812,0.09375,-0.273438,-0.820312,0.0859375,-0.273438,0.796875,0.203125,-0.210938,0.835938,0.171875,-0.273438,-0.796875,0.203125,-0.210938,0.890625,0.242187,-0.265625,0.84375,0.289062,-0.210938,-0.890625,0.242187,-0.265625,-0.835938,0.171875,-0.273438,-0.84375,0.289062,-0.210938,0.890625,0.234375,-0.320312,0.953125,0.289062,-0.34375,-0.890625,0.234375,-0.320312,-0.953125,0.289062,-0.34375,-0.84375,0.171875,-0.320312,0.765625,0.09375,-0.320312,0.84375,0.171875,-0.320312,-0.765625,0.09375,-0.320312,-0.828125,0.078125,-0.320312,0.828125,0.078125,-0.320312,-0.851562,0.015625,-0.320312,0.8125,-0.015625,-0.320312,0.851562,0.015625,-0.320312,-0.8125,-0.015625,-0.320312,0.882812,-0.015625,-0.265625,-0.882812,-0.015625,-0.265625,1.03906,0.328125,-0.414062,-1.03906,0.328125,-0.414062,1.1875,0.34375,-0.484375,-1.1875,0.34375,-0.484375,1.25781,0.242187,-0.492188,-1.25781,0.242187,-0.492188,1.21094,0.0859375,-0.484375,1.1875,0.09375,-0.445312,-1.21094,0.0859375,-0.484375,1.04688,-1.84407e-08,-0.421875,-1.04688,-1.84407e-08,-0.421875,0.890625,0.109375,-0.328125,-0.890625,0.109375,-0.328125,-0.9375,0.0625,-0.335938,0.9375,0.0625,-0.335938,0.960938,0.171875,-0.351562,-0.960938,0.171875,-0.351562,-1,0.125,-0.367188,1.05469,0.1875,-0.382812,1.01562,0.234375,-0.375,-1.05469,0.1875,-0.382812,-1.01562,0.234375,-0.375,1.08594,0.273437,-0.390625,-1.08594,0.273437,-0.390625,-1.10938,0.210937,-0.390625,1.10938,0.210937,-0.390625,0.789062,-0.125,-0.328125,1.03906,-0.0859375,-0.492188,-0.789062,-0.125,-0.328125,-1.03906,-0.0859375,-0.492188,1.3125,0.0546875,-0.53125,-1.3125,0.0546875,-0.53125,1.36719,0.296875,-0.5,-1.36719,0.296875,-0.5,1.25,0.46875,-0.546875,-1.25,0.46875,-0.546875,1.02344,0.4375,-0.484375,-1.02344,0.4375,-0.484375,0.859375,0.382812,-0.382813,-0.859375,0.382812,-0.382813,-0.773438,0.265625,-0.4375,-0.164062,0.414063,0.773438,1,0.125,-0.367188 };
		constexpr uint indices[] = { 0,1,2,3,4,5,2,6,7,8,5,9,1,10,6,11,3,8,12,13,1,14,15,3,16,17,13,18,19,14,13,20,10,21,14,11,22,20,17,23,21,24,25,17,26,27,18,23,25,28,22,29,27,23,28,30,22,29,24,31,32,33,28,34,31,35,36,28,37,38,29,34,39,32,36,40,34,41,42,43,32,41,35,44,2,45,42,5,44,9,0,42,39,4,41,5,39,46,0,40,47,48,36,49,39,38,48,50,37,51,36,52,50,53,25,54,37,27,53,55,25,56,57,58,27,55,26,59,56,60,61,58,16,62,59,63,19,60,12,46,62,47,15,63,64,62,46,47,63,65,59,62,64,65,63,60,64,56,59,60,58,65,64,57,56,58,55,65,64,54,57,55,53,65,64,51,54,53,50,65,64,49,51,50,48,65,64,46,49,48,47,65,66,67,68,69,67,70,71,68,72,73,74,69,75,71,72,73,76,77,78,79,75,80,81,76,82,83,78,84,85,81,86,87,88,89,90,91,92,87,93,94,89,95,92,96,97,98,94,95,99,96,100,101,98,102,103,104,99,105,102,106,107,108,103,109,106,110,107,111,112,113,109,110,114,111,115,116,113,117,118,119,114,120,117,121,122,123,118,122,121,124,125,123,126,127,121,117,125,111,119,113,127,117,112,128,129,110,130,113,108,129,131,106,132,110,104,131,133,102,134,106,96,133,135,98,136,102,97,135,137,95,138,98,87,137,139,89,140,95,17,87,139,89,18,141,17,142,88,142,18,91,123,143,126,121,144,124,143,145,146,145,144,147,148,145,142,149,145,147,150,70,66,70,151,69,152,66,71,69,153,73,152,79,154,153,80,73,155,156,83,157,158,84,154,83,156,84,159,157,160,161,162,160,163,164,161,165,156,163,165,164,154,165,166,159,165,157,167,168,162,169,170,171,172,167,173,174,170,175,176,173,177,178,179,174,180,177,181,180,177,178,162,182,160,171,182,169,168,183,182,169,183,184,180,185,176,186,180,178,176,187,172,188,178,174,187,189,172,188,175,190,189,191,168,184,175,169,192,185,193,190,186,188,193,191,192,184,193,190,177,88,142,91,177,142,173,194,88,195,179,91,162,194,167,171,195,196,161,197,162,163,196,158,198,155,82,199,158,196,200,197,198,201,196,195,86,194,200,195,90,201,166,202,154,166,203,204,152,202,205,203,153,206,150,205,207,206,151,208,209,207,210,208,209,210,207,211,210,208,211,212,207,213,214,215,208,212,205,216,213,217,206,215,204,216,202,204,217,218,218,214,216,212,218,217,216,214,213,215,212,217,146,219,220,221,147,222,143,220,223,222,144,224,143,225,126,144,226,224,17,219,148,18,221,227,17,228,229,230,18,227,139,231,228,232,141,230,137,233,231,234,140,232,135,235,233,236,138,234,131,235,133,134,236,237,129,238,131,132,237,239,129,240,241,242,132,239,128,243,240,244,130,242,125,225,243,226,127,244,243,245,246,247,244,248,240,246,249,248,242,250,240,251,241,242,252,250,241,253,238,239,254,252,235,253,255,254,236,256,235,257,233,236,258,256,231,257,259,258,232,260,231,261,228,232,262,260,228,263,229,230,264,262,219,263,265,264,221,266,225,267,245,268,226,247,223,269,267,270,224,268,220,265,269,266,222,270,122,271,272,273,122,272,118,274,271,275,120,273,115,274,114,276,275,277,107,278,115,109,277,279,103,280,107,105,279,281,103,282,283,284,105,281,100,282,99,285,284,286,100,287,288,289,285,286,92,290,287,291,94,289,292,293,294,292,295,296,294,297,298,294,299,295,298,300,301,298,302,299,68,301,300,301,74,302,72,300,303,302,77,304,75,303,305,304,76,306,78,305,307,306,81,308,305,293,307,295,306,308,303,297,305,304,299,302,307,309,310,308,296,295,82,307,310,308,85,311,312,200,198,313,201,314,310,198,82,311,199,313,200,315,86,201,316,314,315,93,86,316,317,291,318,319,320,321,319,322,323,324,319,322,324,325,324,326,327,328,324,327,327,309,292,296,327,292,309,312,310,296,313,328,288,329,330,331,286,332,333,320,334,335,320,321,336,337,338,339,337,340,337,341,342,343,337,342,342,333,334,335,342,334,283,344,345,346,281,347,345,348,349,350,347,351,349,352,353,354,351,355,353,356,357,358,355,359,360,356,361,362,358,359,333,357,360,359,335,362,341,353,357,355,343,359,363,349,353,351,340,355,336,345,349,347,339,351,283,364,280,281,365,347,364,338,366,365,338,339,274,280,271,275,279,277,271,364,366,365,273,366,272,271,366,366,273,272,288,344,282,286,346,332,330,348,344,350,332,346,367,352,348,354,368,350,356,369,361,358,370,354,371,372,326,325,373,374,372,375,329,373,376,374,287,372,329,373,289,331,290,312,372,313,291,373,312,326,372,373,328,313,290,315,377,314,316,291,378,360,361,379,362,380,360,318,333,362,321,380,381,378,375,374,379,380,323,381,371,322,374,380,318,382,323,322,380,321,383,384,385,386,387,388,385,389,390,391,392,393,389,394,390,391,395,396,397,398,394,396,399,400,401,402,398,400,403,404,402,405,406,407,403,408,409,410,405,411,404,407,401,412,409,413,400,404,414,401,397,415,400,416,417,397,389,418,396,415,419,389,384,420,391,418,384,421,419,422,387,420,375,423,329,376,424,425,406,426,375,408,425,407,330,423,367,424,332,368,369,427,383,388,370,386,405,428,426,429,407,425,430,428,431,432,429,425,433,431,434,435,436,437,438,433,439,440,436,432,438,441,442,440,443,444,442,421,427,445,422,443,438,369,367,440,370,445,423,438,367,424,440,432,423,426,430,432,425,424,421,446,447,448,422,449,439,446,441,444,448,450,439,451,452,453,444,450,434,451,433,437,453,454,431,455,434,435,454,456,431,457,458,459,435,456,428,460,457,461,429,459,419,447,462,449,420,463,417,462,464,463,418,465,414,464,466,465,415,467,414,468,469,415,470,467,469,471,412,416,472,470,412,460,410,413,461,472,458,473,455,456,474,475,476,477,473,475,478,479,477,480,481,482,478,483,480,484,481,482,485,486,462,481,484,483,463,485,477,447,446,478,449,483,452,477,446,450,478,474,455,452,451,450,454,453,460,458,457,461,456,475,471,476,460,475,472,461,480,471,468,482,472,479,487,468,466,486,470,482,464,487,466,486,465,467,462,484,464,465,485,463,402,488,489,490,403,491,398,489,492,491,399,493,398,494,394,399,495,493,394,496,390,395,497,495,390,498,385,393,499,497,385,500,383,392,501,499,489,500,498,491,501,490,498,492,489,493,499,491,496,494,492,493,495,497,369,500,361,370,501,386,361,488,378,490,502,379,375,488,406,490,376,408,0,12,1,3,15,4,2,1,6,8,3,5,1,13,10,11,14,3,12,16,13,14,19,15,16,26,17,18,61,19,13,17,20,21,18,14,22,30,20,23,18,21,25,22,17,27,61,18,25,37,28,29,52,27,28,33,30,29,23,24,32,43,33,34,29,31,36,32,28,38,52,29,39,42,32,40,38,34,42,45,43,41,34,35,2,7,45,5,41,44,0,2,42,4,40,41,39,49,46,40,4,47,36,51,49,38,40,48,37,54,51,52,38,50,25,57,54,27,52,53,25,26,56,58,61,27,26,16,59,60,19,61,16,12,62,63,15,19,12,0,46,47,4,15,66,70,67,69,74,67,71,66,68,73,77,74,75,79,71,73,80,76,78,83,79,80,84,81,82,155,83,84,158,85,86,93,87,89,317,90,92,97,87,94,317,89,92,100,96,98,285,94,99,104,96,101,285,98,103,108,104,105,101,102,107,112,108,109,105,106,107,115,111,113,276,109,114,119,111,116,276,113,118,123,119,120,116,117,122,124,123,122,120,121,125,119,123,127,503,121,125,128,111,113,130,127,112,111,128,110,132,130,108,112,129,106,134,132,104,108,131,102,136,134,96,104,133,98,138,136,97,96,135,95,140,138,87,97,137,89,141,140,17,88,87,89,91,18,17,148,142,142,149,18,123,124,143,121,503,144,143,124,145,145,124,144,148,146,145,149,142,145,150,209,70,70,209,151,152,150,66,69,151,153,152,71,79,153,159,80,155,161,156,157,163,158,154,79,83,84,80,159,160,164,161,160,171,163,161,164,165,163,157,165,154,156,165,159,166,165,167,189,168,169,175,170,172,189,167,174,179,170,176,172,173,178,177,179,180,176,177,162,168,182,171,160,182,168,191,183,169,182,183,180,193,185,186,193,180,176,185,187,188,186,178,187,192,189,188,174,175,189,192,191,184,190,175,192,187,185,190,193,186,193,183,191,184,183,193,177,173,88,91,179,177,173,167,194,195,170,179,162,197,194,171,170,195,161,155,197,163,171,196,198,197,155,199,85,158,200,194,197,201,199,196,86,88,194,195,91,90,166,204,202,166,159,203,152,154,202,203,159,153,150,152,205,206,153,151,209,150,207,208,151,209,207,214,211,208,210,211,207,205,213,215,206,208,205,202,216,217,203,206,204,218,216,204,203,217,218,211,214,212,211,218,146,148,219,221,149,147,143,146,220,222,147,144,143,223,225,144,503,226,17,229,219,18,149,221,17,139,228,230,141,18,139,137,231,232,140,141,137,135,233,234,138,140,135,133,235,236,136,138,131,238,235,134,136,236,129,241,238,132,134,237,129,128,240,242,130,132,128,125,243,244,127,130,125,126,225,226,503,127,243,225,245,247,226,244,240,243,246,248,244,242,240,249,251,242,239,252,241,251,253,239,237,254,235,238,253,254,237,236,235,255,257,236,234,258,231,233,257,258,234,232,231,259,261,232,230,262,228,261,263,230,227,264,219,229,263,264,227,221,225,223,267,268,224,226,223,220,269,270,222,224,220,219,265,266,221,222,122,118,271,273,120,122,118,114,274,275,116,120,115,278,274,276,116,275,107,280,278,109,276,277,103,283,280,105,109,279,103,99,282,284,101,105,100,288,282,285,101,284,100,92,287,289,94,285,92,93,290,291,317,94,292,309,293,292,294,295,294,293,297,294,298,299,298,297,300,298,301,302,68,67,301,301,67,74,72,68,300,302,74,77,75,72,303,304,77,76,78,75,305,306,76,81,305,297,293,295,299,306,303,300,297,304,306,299,307,293,309,308,311,296,82,78,307,308,81,85,312,377,200,313,199,201,310,312,198,311,85,199,200,377,315,201,90,316,315,290,93,316,90,317,318,323,319,321,320,319,323,371,324,322,319,324,324,371,326,328,325,324,327,326,309,296,328,327,309,326,312,296,311,313,288,287,329,331,289,286,333,318,320,335,334,320,336,363,337,339,338,337,337,363,341,343,340,337,342,341,333,335,343,342,283,282,344,346,284,281,345,344,348,350,346,347,349,348,352,354,350,351,353,352,356,358,354,355,360,357,356,362,502,358,333,341,357,359,343,335,341,363,353,355,340,343,363,336,349,351,339,340,336,364,345,347,365,339,283,345,364,281,279,365,364,336,338,365,366,338,274,278,280,275,273,279,271,280,364,365,279,273,288,330,344,286,284,346,330,367,348,350,368,332,367,369,352,354,370,368,356,352,369,358,502,370,371,381,372,325,328,373,372,381,375,373,331,376,287,290,372,373,291,289,290,377,312,313,314,291,378,382,360,379,502,362,360,382,318,362,335,321,381,382,378,374,376,379,323,382,381,322,325,374,383,427,384,386,392,387,385,384,389,391,387,392,389,397,394,391,393,395,397,401,398,396,395,399,401,409,402,400,399,403,402,409,405,407,404,403,409,412,410,411,413,404,401,469,412,413,416,400,414,469,401,415,396,400,417,414,397,418,391,396,419,417,389,420,387,391,384,427,421,422,388,387,375,426,423,376,331,424,406,405,426,408,376,425,330,329,423,424,331,332,369,442,427,388,445,370,405,410,428,429,411,407,430,426,428,432,435,429,433,430,431,435,432,436,438,430,433,440,444,436,438,439,441,440,445,443,442,441,421,445,388,422,438,442,369,440,368,370,423,430,438,424,368,440,421,441,446,448,443,422,439,452,446,444,443,448,439,433,451,453,436,444,434,455,451,437,436,453,431,458,455,435,437,454,431,428,457,459,429,435,428,410,460,461,411,429,419,421,447,449,422,420,417,419,462,463,420,418,414,417,464,465,418,415,414,466,468,415,416,470,469,468,471,416,413,472,412,471,460,413,411,461,458,476,473,456,454,474,476,504,477,475,474,478,477,504,480,482,479,478,480,487,484,482,483,485,462,447,481,483,449,463,477,481,447,478,448,449,452,473,477,450,448,478,455,473,452,450,474,454,460,476,458,461,459,456,471,504,476,475,479,472,480,504,471,482,470,472,487,480,468,486,467,470,464,484,487,486,485,465,402,406,488,490,408,403,398,402,489,491,403,399,398,492,494,399,395,495,394,494,496,395,393,497,390,496,498,393,392,499,385,498,500,392,386,501,489,488,500,491,499,501,498,496,492,493,497,499,369,383,500,370,502,501,361,500,488,490,501,502,375,378,488,490,379,376 };

		span<double3> points = { (double3*)position, array_size(position) / 3 };
		span<uint3> faces = { (uint3*)indices, array_size(indices) / 3 };

		MeshPacker shape;

		for(uint3 face : faces)
		{
			const vec3 v0 = vec3(points[face[0]]);
			const vec3 v1 = vec3(points[face[1]]);
			const vec3 v2 = vec3(points[face[2]]);

			const vec3 normal = Plane(v0, v1, v2).m_normal;

			shape.position(v0);
			shape.position(v1);
			shape.position(v2);

			shape.normal(normal);
			shape.normal(normal);
			shape.normal(normal);
		}

		Model& model = gfx.create_model_geo("suzanne", shape);
		return model;
	}
}
}
