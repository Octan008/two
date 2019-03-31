// effect_godrays.js

// @author huwb / http://huwbowles.com/
// @ref Sousa2008 - Crysis Next Gen Effects, GDC2008, http://www.crytek.com/sites/default/files/GDC08_SousaT_CrysisEffects.ppt

var filter_vertex = `$input a_position, a_texcoord0
		$output v_uv0
		
		#include <filter.sh>
		
		void main() {
			v_uv0 = u_source_crop.xy + a_texcoord0 * u_source_crop.zw;
			gl_Position = mul(u_modelViewProj, vec4(a_position.xyz, 1.0));
		}`;
        
var godrays_mask_vertex = filter_vertex;

var godrays_mask_fragment = `$input v_uv0
		
		#include <filter.sh>
		
		void main() {
			float depth = texture2D(s_source_depth, v_uv0).x;
			depth = -perspectiveDepthToViewZ(depth) / u_z_far;
			depth = clamp(depth, 0.0, 1.0);
			gl_FragColor = vec4(1.0 - depth, 0.0, 0.0, 0.0);
		//	gl_FragColor = vec4(1.0) - texture2D(s_source_0, v_uv0);
		}`;

var godrays_vertex = filter_vertex;

var godrays_fragment = `$input v_uv0
		
		#include <filter.sh>
		
		#define TAPS_PER_PASS 6.0
		
		uniform vec4 u_godrays_p0;
		#define u_sun u_godrays_p0.xy
		#define u_step_size u_godrays_p0.z
		
		void main() {

			// delta from current pixel to sun position
			vec2 delta = u_sun - v_uv0;
			float dist = length(delta);

			// Step vector (uv space)
			vec2 stepv = u_step_size * delta / dist;

			// Number of iterations between pixel and sun
			float iters = dist / u_step_size;

			vec2 uv = v_uv0;
			float col = 0.0;

			// This breaks ANGLE in Chrome 22
			//	- see http://code.google.com/p/chromium/issues/detail?id=153105

			/*
			// Unrolling didnt do much on my hardware (ATI Mobility Radeon 3450),
			// so ive just left the loop

			for (float i = 0.0; i < TAPS_PER_PASS; i += 1.0) {

				// Accumulate samples, making sure we dont walk past the light source.

				// The check for uv.y < 1 would not be necessary with border UV wrap
				// mode, with a black border color. I dont think this is currently
				// exposed by three.js. As a result there might be artifacts when the
				// sun is to the left, right or bottom of screen as these cases are
				// not specifically handled.

				col += (i <= iters  uv.y < 1.0 ? texture2D(s_source_0, uv).r : 0.0);
				uv += stepv;

			}
			*/

			// Unrolling loop manually makes it work in ANGLE

			if (0.0 <= iters  uv.y < 1.0) col += texture2D(s_source_0, uv).r;
			uv += stepv;

			if (1.0 <= iters  uv.y < 1.0) col += texture2D(s_source_0, uv).r;
			uv += stepv;

			if (2.0 <= iters  uv.y < 1.0) col += texture2D(s_source_0, uv).r;
			uv += stepv;

			if (3.0 <= iters  uv.y < 1.0) col += texture2D(s_source_0, uv).r;
			uv += stepv;

			if (4.0 <= iters  uv.y < 1.0) col += texture2D(s_source_0, uv).r;
			uv += stepv;

			if (5.0 <= iters  uv.y < 1.0) col += texture2D(s_source_0, uv).r;
			uv += stepv;

			// Should technically be dividing by iters, but TAPS_PER_PASS smooths out
			// objectionable artifacts, in particular near the sun position. The side
			// effect is that the result is darker than it should be around the sun, as
			// TAPS_PER_PASS is greater than the number of samples actually accumulated.
			// When the result is inverted (in the shader godrays_combine, this produces
			// a slight bright spot at the position of the sun, even when it is occluded.

			gl_FragColor = vec4(col/TAPS_PER_PASS, 0.0, 0.0, 1.0);

		}`;

var godrays_combine_vertex = filter_vertex;

var godrays_combine_fragment = `$input v_uv0
		
		#include <filter.sh>
		
		uniform vec4 u_godrays_combine_p0;
		#define u_intensity u_godrays_combine_p0.x
		
		void main() {
		
		// Since THREE.MeshDepthvar renders foreground objects white and background
		// objects black, the god-rays will be white streaks. Therefore value is inverted
		// before being combined with tColors
		
			float amount = u_intensity * (1.0 - texture2D(s_source_1, v_uv0).r);
			gl_FragColor = texture2D(s_source_0, v_uv0) + vec4_splat(amount);
			gl_FragColor.a = 1.0;
		//	gl_FragColor = texture2D(s_source_0, v_uv0);
		
		}`;

var godrays_sun_vertex = filter_vertex;

var godrays_sun_fragment = `$input v_uv0
		
		#include <filter.sh>
		
		uniform vec4 u_godrays_sun_p0;
		#define u_bg_color u_godrays_sun_p0.xyz
		
		uniform vec4 u_godrays_sun_p1;
		#define u_sun_color u_godrays_sun_p1.xyz
		
		uniform vec4 u_godrays_sun_p2;
		#define u_sun_xy u_godrays_sun_p2.xy
		
		void main() {
		
			vec2 diff = v_uv0 - u_sun_xy;
		// Correct for aspect ratio
			diff.x *= u_aspect;
		
			float prop = clamp(length(diff) / 0.5, 0.0, 1.0);
			prop = 0.35 * pow(1.0 - prop, 3.0);
		
			gl_FragColor.xyz = mix(u_sun_color, u_bg_color, 1.0 - prop);
			gl_FragColor.w = 1.0;
		}`;

function pass_fake_sun(gfx, render, godrays) {
    
    var program = gfx.programs.fetch('godrays_fake_sun')
	var pass = render.next_pass('godrays_fake_sun', two.PassType.PostProcess);

	gfx.filter.uniform(pass, 'u_godrays_sun_p0', new two.vec4(godrays.bg_colour.x, godrays.bg_colour.y, godrays.bg_colour.z, 0.0));
	gfx.filter.uniform(pass, 'u_godrays_sun_p1', new two.vec4(godrays.sun_colour.x, godrays.sun_colour.y, godrays.sun_colour.z, 0.0));
	gfx.filter.uniform(pass, 'u_godrays_sun_p2', new two.vec4(godrays.sun_screen.x, godrays.sun_screen.y, 0.0, 0.0));

	var sunsqH = 0.74 * render.rect.height; // 0.74 depends on extent of sun from shader
	var sunsqW = sunsqH; // both depend on height because sun is aspect-corrected

	//var sun = new two.vec2(rect_offset(render.rect)) + godrays.sun_screen * vec2(rect_size(render.rect));

	// @todo fix this
	//btwo.gfx.setViewScissor(pass.index, sun.x - sunsqW / 2.0, sun.y - sunsqH / 2.0, sunsqW, sunsqH);

	gfx.filter.quad(pass, render.target_fbo, program, pass.viewport.rect);
}

void pass_godrays(gfx, render, godrays) {
	//// Use a smaller size for some of the god-ray render targets for better performance.
	var downscale = 1.0 / 4.0;
	var size = rect_size(render.rect) / 4;

	// The ping-pong render targets can use an adjusted resolution to minimize cost
	var ping = new two.FrameBuffer(render.target.size / 4, btwo.gfx.TextureFormat::R32F, 0);
	var pong = new two.FrameBuffer(render.target.size / 4, btwo.gfx.TextureFormat::R32F, 0);
	
	// I would have this quarter size and use it as one of the ping-pong render
	// targets but the aliasing causes some temporal flickering
	var depth = new two.FrameBuffer(render.target.size, btwo.gfx.TextureFormat::R32F, 0);

	function pass_mask_depth(gfx, render, godrays, fbo) {
        
		var program = gfx.programs.fetch('godrays_depth_mask');

		var pass = render.next_pass('godrays_depth_mask', two.PassType.PostProcess);

		gfx.filter.sourcedepth(render.target.depth);

		gfx.filter.quad(pass, fbo, program, pass.viewport.rect);
	}

	function pass_blur(gfx, render, godrays, fbo, source, step_size, quad) {
		var program = gfx.programs.fetch('godrays_generate');

		var pass = render.next_pass('godrays', two.PassType.PostProcess);

		gfx.filter.uniform(pass, 'u_godrays_p0', new two.vec4(godrays.sun_screen.x, godrays.sun_screen.y, step_size, 0.0));
		gfx.filter.source0(source, GFX_TEXTURE_CLAMP);

		gfx.filter.quad(pass, fbo, program, quad);
	}

	function pass_combine(gfx, render, godrays, source) {
        
		var program = gfx.programs.fetch('godrays_combine');

		var pass = render.next_pass('godrays_combine', two.PassType.PostProcess);

		gfx.filter.source0(render.target.diffuse);
		gfx.filter.source1(source);

		gfx.filter.uniform(pass, 'u_godrays_combine_p0', new two.vec4(godrays.intensity, 0.0, 0.0, 0.0));

		gfx.filter.quad(pass, render.target.post_process.swap(), program, pass.viewport.rect);

        var flip = render.next_pass('flip', two.PassType.PostProcess);
		gfx.copy.quad(flip, render.target_fbo, render.target.post_process.last(), pass.viewport.rect);
	}

	pass_mask_depth(gfx, render, godrays, depth);

	// Render god-rays

	// Maximum length of god-rays (in texture space [0,1]X[0,1])
	var filter_length = 1.0;
	//var filter_length = 0.5;

	// Samples taken by filter
	var taps = 6.0;

	// Pass order could equivalently be 3,2,1 (instead of 1,2,3), which
	// would start with a small filter support and grow to large. however
	// the large-to-small order produces less objectionable aliasing artifacts that
	// appear as a glimmer along the length of the beams

	function step_size(filter_length, taps, pass) {
		return filter_length * pow(taps, -pass);
	}

	var rect = new two.vec4(render.rect);
	var rect4 = rect / 4.0;

	// pass 1 - render into first ping-pong target
	var quad0 = new two.RenderQuad(depth.source_quad(rect, true), pong.dest_quad(rect4, true), true);
	pass_blur(gfx, render, godrays, pong, depth.tex, step_size(filter_length, taps, 1.0), quad0);

	// pass 2 - render into second ping-pong target
	var quad0 = new two.RenderQuad(pong.source_quad(rect4, true), ping.dest_quad(rect4, true), true);
	pass_blur(gfx, render, godrays, ping, pong.tex, step_size(filter_length, taps, 2.0), quad1);

	// pass 3 - 1st RT
	var quad0 = new two.RenderQuad(ping.source_quad(rect4, true), pong.dest_quad(rect4, true), true);
	pass_blur(gfx, render, godrays, pong, ping.tex, step_size(filter_length, taps, 3.f), quad2);

	// final pass - composite god-rays onto colors
	pass_combine(gfx, render, godrays, pong.tex);

	//copy.debug_show_texture(render, depth, vec4(0.0));
	//gfx.copy.debug_show_texture(render, pong.tex, vec4(0.0));
}

var viewer = two.ui.scene_viewer(panel);
//two.ui.orbit_controls(viewer);

var scene = viewer.scene;

var orbit_radius = 200.0;

var godrays = { intensity: 0.75, bg_colour: two.rgb(0x000511), sun_colour: two.rgb(0xffee00), 
                sun_position: new two.vec3(0, 1000.0, -1000.0) };


var zeroq = new two.quat(new two.vec3(0.0));
    
if(init) {
    //ImporterOBJ obj_importer(app.gfx);

    this.mouse = new two.vec2(0.0);
    
    var camera = viewer.camera;
    camera.fov = 70.0; camera.near = 1.0; camera.far = 3000.0;
    camera.eye.z = 200.0;

    var fake_sun = gfx.programs.create('godrays_fake_sun');
    fake_sun.set_source(two.ShaderType.Vertex, godrays_sun_vertex);
    fake_sun.set_source(two.ShaderType.Fragment, godrays_sun_fragment);
    
    var depth_mask = gfx.programs.create('godrays_depth_mask');
    depth_mask.set_source(two.ShaderType.Vertex, godrays_mask_vertex);
    depth_mask.set_source(two.ShaderType.Fragment, godrays_mask_fragment);

    var generate = gfx.programs.create('godrays_generate');
    generate.set_source(two.ShaderType.Vertex, godrays_vertex);
    generate.set_source(two.ShaderType.Fragment, godrays_fragment);

    var combine = gfx.programs.create('godrays_combine');
    combine.set_source(two.ShaderType.Vertex, godrays_combine_vertex);
    combine.set_source(two.ShaderType.Fragment, godrays_combine_fragment);

    var solid = app.gfx.programs.fetch('solid');

    viewer.viewport.clear_colour = godrays.bg_colour;

    var material = app.gfx.materials.create('godrays'); var m = material;
        m.program = solid;
        m.base.depth_draw = two.DepthDraw.Enabled;
        m.solid.colour = two.rgb(0x000000);
        //m.solid.colour = two.rgb(0xffffff);

    var model = app.gfx.models.file('tree'); // tree.obj
    model.items[0].material = material;

    var ntree = scene.nodes().add(new two.Node3(new two.vec3(0.0, -150.0, -150.0), zeroq, new two.vec3(400.0)));
    scene.items().add(new two.Item(ntree, model, 0, material));

    var sphere = app.gfx.shape(new two.Sphere(1.0));
    var nsphere = scene.nodes().add(new two.Node3(new two.vec3(0.0), zeroq, new two.vec3(20.0)));
    scene.items().add(new two.Item(nsphere, sphere, 0, material));
    this.node = nsphere;

    viewer.viewport.clear_colour = godrays.bg_colour;
}

//var root = viewer.scene.begin();
//two.gfx.radiance(root, 'radiance/tiber_1_1k.hdr', BackgroundMode::Radiance);

var event = viewer.mouse_event(two.DeviceType.Mouse, two.EventType.Moved);
if(event.valid())
{
    mouse.x = event.relative.x - viewer.frame.size.x / 2.0;
    mouse.y = event.relative.y - viewer.frame.size.y / 2.0;
}

var camera = viewer.camera;
camera.eye.x += (mouse.x - camera.eye.x) * 0.036;
camera.eye.y += (-(mouse.y) - camera.eye.y) * 0.036;

var timer = app.gfx.time * 0.5;

var position = new two.vec3(0.0);
position.x = orbit_radius * Math.cos(timer);
position.z = orbit_radius * Math.sin(timer) - 100.0;

node.apply(position, zeroq, new two.vec3(20.0));

// Find the screenspace position of the sun
const mat4 viewproj = camera.projection * camera.transform;
vec4 sun_clip = viewproj * vec4(godrays.sun_position, 1.0);
vec3 sun_ndc = vec3(sun_clip) / sun_clip.w;
vec3 sun = (sun_ndc + 1.0) / 2.0;
if(!app.gfx.flip_y)
    sun.y = 1.0 - sun.y;

godrays.sun_screen = new two.vec2(sun.x, sun.y);

function renderer(gfx, render) {
    
    two.begin_pbr_render(gfx, render);

    two.pass_clear(gfx, render);
    pass_fake_sun(gfx, render, godrays);
    
    //two.pass_opaque(gfx, render);
    two.pass_solid(gfx, render);
    
    pass_godrays(gfx, render, godrays);
}

app.gfx.set_renderer(Shading::Shaded, render);
