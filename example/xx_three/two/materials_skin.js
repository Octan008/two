//#include <mud/frame.h>
#include <frame/Api.h>
#include <gfx-pbr/Api.h>
#include <gfx-gltf/Api.h>

#include <xx_three/xx_three.h>

#include <stl/vector.hpp>

using namespace mud;

//<script src = 'js/ShaderSkin.js'>< / script>
//
//<script src = 'js/shaders/BleachBypassShader.js'>< / script>
//<script src = 'js/shaders/ConvolutionShader.js'>< / script>
//<script src = 'js/shaders/CopyShader.js'>< / script>
//
//<script src = 'js/postprocessing/EffectComposer.js'>< / script>
//<script src = 'js/postprocessing/RenderPass.js'>< / script>
//<script src = 'js/postprocessing/BloomPass.js'>< / script>
//<script src = 'js/postprocessing/TexturePass.js'>< / script>
//<script src = 'js/postprocessing/ShaderPass.js'>< / script>
//<script src = 'js/postprocessing/MaskPass.js'>< / script>

void xx_materials_skin(Shell app, var parent, Dockbar dockbar)
{
	ImporterGltf importer_gltf = { app.gfx };

	var viewer = two.ui.scene_viewer(app.ui.begin());
	//two.ui.orbit_controller(viewer);

	var scene = viewer.scene;

	bool firstPass = true;

	this.mesh = nullptr;
	this.rotation = new two.vec3(0.0);

	bool once = false;
	if(!once)
	{
		once = true;

		var camera = viewer.camera;
		camera.fov = 35.0; camera.near = 1.0; camera.far = 10000.0;
		camera.eye.z = 900.0;

		//scene.background = new THREE.Color(0x050505);

		var ln0 = scene.nodes().add(new two.Node3(new two.vec3(0.0), facing(normalize(new two.vec3(-1.0, -0.5, -1.0)))));
		Light l0 = scene.lights().add(new two.Light(ln0, LightType::Direct, false, rgb(0xffeedd), 1.5));

		var ln1 = scene.nodes().add(new two.Node3(new two.vec3(0.0), facing(normalize(new two.vec3(1.0, -0.5, 1.0)))));
		Light l1 = scene.lights().add(new two.Light(ln0, LightType::Direct, false, rgb(0xddddff), 0.5));

		// MATERIALS

		var diffuse = rgb(0xbbbbbb);
		var specular = rgb(0x555555);

		//var shader = THREE.ShaderSkin['skin'];

		//var uniformsUV = THREE.UniformsUtils.clone(shader.uniforms);

		//uniformsUV['tNormal'].value = textureLoader.load('models/gltf/LeePerrySmith/Infinite-Level_02_Tangent_SmoothUV.jpg');
		//uniformsUV['uNormalScale'].value = -1.5;
		//
		//uniformsUV['tDiffuse'].value = textureLoader.load('models/gltf/LeePerrySmith/Map-COL.jpg');
		//
		//uniformsUV['passID'].value = 0;
		//
		//uniformsUV['diffuse'].value.setHex(diffuse);
		//uniformsUV['specular'].value.setHex(specular);
		//
		//uniformsUV['uRoughness'].value = 0.185;
		//uniformsUV['uSpecularBrightness'].value = 0.7;


		//var uniforms = THREE.UniformsUtils.clone(uniformsUV);
		//uniforms['tDiffuse'].value = uniformsUV['tDiffuse'].value;
		//uniforms['tNormal'].value = uniformsUV['tNormal'].value;
		//uniforms['passID'].value = 1;

		//var parameters = { fragmentShader: shader.0ragmentShader, vertexShader : shader.vertexShader, uniforms : uniforms, lights : true };
		//var parametersUV = { fragmentShader: shader.0ragmentShader, vertexShader : shader.vertexShaderUV, uniforms : uniformsUV, lights : true };
		//
		//var material = new THREE.ShaderMaterial(parameters);
		//material.extensions.derivatives = true;
		//
		//var materialUV = new THREE.ShaderMaterial(parametersUV);
		//materialUV.extensions.derivatives = true;


		var model = app.gfx.models.file('LeePerrySmith.glb');
		//.then(createScene(gltf.scene.children[0].geometry, 100, material);

		var n = scene.nodes().add(new two.Node3(new two.vec3(0.0, -50.0, 0.0), ZeroQuat, new two.vec3(100.0)));
		//scene.items().add(new two.Item(n, model, 0, material));

		// POSTPROCESSING
		//var renderModelUV = new THREE.RenderPass(scene, camera, materialUV, new THREE.Color(0x575757));
		//
		//var effectCopy = new THREE.ShaderPass(THREE.CopyShader);
		//
		//var effectBloom1 = new THREE.BloomPass(1, 15, 2, 512);
		//var effectBloom2 = new THREE.BloomPass(1, 25, 3, 512);
		//var effectBloom3 = new THREE.BloomPass(1, 25, 4, 512);
		//
		//effectBloom1.clear = true;
		//effectBloom2.clear = true;
		//effectBloom3.clear = true;
		//
		//effectCopy.renderToScreen = true;

		//

		//var pars = {
		//	generateMipmaps: true,
		//	minFilter : THREE.LinearMipmapLinearFilter,
		//	magFilter : THREE.LinearFilter,
		//	format : THREE.RGBFormat,
		//	stencilBuffer : false
		//};
		//
		//var rtwidth = 512;
		//var rtheight = 512;
		//
		////
		//
		//composer = new THREE.EffectComposer(renderer, new THREE.WebGLRenderTarget(rtwidth, rtheight, pars));
		//composer.addPass(renderModelUV);
		//
		//var renderScene = new THREE.TexturePass(composer.renderTarget2.texture);

		//

		//composerUV1 = new THREE.EffectComposer(renderer, new THREE.WebGLRenderTarget(rtwidth, rtheight, pars));
		//
		//composerUV1.addPass(renderScene);
		//composerUV1.addPass(effectBloom1);
		//
		//composerUV2 = new THREE.EffectComposer(renderer, new THREE.WebGLRenderTarget(rtwidth, rtheight, pars));
		//
		//composerUV2.addPass(renderScene);
		//composerUV2.addPass(effectBloom2);
		//
		//composerUV3 = new THREE.EffectComposer(renderer, new THREE.WebGLRenderTarget(rtwidth, rtheight, pars));
		//
		//composerUV3.addPass(renderScene);
		//composerUV3.addPass(effectBloom3);

		//

		//var effectBeckmann = new THREE.ShaderPass(THREE.ShaderSkin['beckmann']);
		//composerBeckmann = new THREE.EffectComposer(renderer, new THREE.WebGLRenderTarget(rtwidth, rtheight, pars));
		//composerBeckmann.addPass(effectBeckmann);

		//

		//uniforms['tBlur1'].value = composer.renderTarget2.texture;
		//uniforms['tBlur2'].value = composerUV1.renderTarget2.texture;
		//uniforms['tBlur3'].value = composerUV2.renderTarget2.texture;
		//uniforms['tBlur4'].value = composerUV3.renderTarget2.texture;
		//
		//uniforms['tBeckmann'].value = composerBeckmann.renderTarget1.texture;
	}

	this.mouse = new two.vec2(0.0);
	if(var event = viewer.mouse_event(two.DeviceType.Mouse, two.EventType.Moved))
	{
		mouse = event.relative - viewer.frame.size / 2.0;
	}

	var target = mouse * 0.001;

	rotation.x += 0.05 * (target.x - rotation.y);
	rotation.y += 0.05 * (target.y - rotation.y);

	mesh.apply(new two.vec3(100.0), new two.quat(rotation), new two.vec3(0.0, -50.0, 0.0));


	//renderer.clear();
	//
	//if(firstPass) {
	//
	//	composerBeckmann.render();
	//	firstPass = false;
	//
	//}
	//
	//composer.render();
	//
	//composerUV1.render();
	//composerUV2.render();
	//composerUV3.render();
	//
	//renderer.render(scene, camera);
}