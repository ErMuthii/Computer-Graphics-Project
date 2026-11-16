import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import { createDressGeometry } from "./dress-geometry.js";
export function createModel(host, pattern, onError) {
  let renderer;
  try {
    renderer = new THREE.WebGLRenderer({
      antialias: true,
      alpha: true,
      preserveDrawingBuffer: true,
    });
  } catch (e) {
    onError(
      "3D needs WebGL. Enable hardware acceleration or use the fabric and photo views.",
    );
    return null;
  }
  renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  renderer.toneMapping = THREE.NeutralToneMapping;
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.PCFSoftShadowMap;
  host.append(renderer.domElement);
  renderer.domElement.setAttribute(
    "aria-label",
    "Interactive 3D mannequin wearing your pattern. Drag to rotate; scroll to zoom.",
  );
  renderer.domElement.tabIndex = 0;
  const scene = new THREE.Scene();
  scene.background = new THREE.Color("#e9e5db");
  const camera = new THREE.PerspectiveCamera(36, 1, 0.1, 100);
  camera.position.set(0, 1.3, 5.7);
  const controls = new OrbitControls(camera, renderer.domElement);
  controls.target.set(0, 1.55, 0);
  controls.enableDamping = true;
  controls.minDistance = 3;
  controls.maxDistance = 9;
  controls.maxPolarAngle = Math.PI * 0.85;
  scene.add(new THREE.HemisphereLight(0xfff8e9, 0x554b3d, 1.8));
  const light = new THREE.DirectionalLight(0xffffff, 2.3);
  light.position.set(3, 5, 4);
  light.castShadow = true;
  light.target.position.set(0, 1.3, 0);
  scene.add(light.target);
  light.shadow.mapSize.set(2048, 2048);
  Object.assign(light.shadow.camera, {
    left: -2.2,
    right: 2.2,
    top: 2.2,
    bottom: -2.2,
    near: 0.5,
    far: 12,
  });
  light.shadow.bias = -0.0003;
  light.shadow.normalBias = 0.02;
  scene.add(light);
  const skin = new THREE.MeshStandardMaterial({
    color: 0xb39d86,
    roughness: 0.8,
  });
  const texture = new THREE.CanvasTexture(pattern);
  texture.colorSpace = THREE.SRGBColorSpace;
  texture.wrapS = texture.wrapT = THREE.RepeatWrapping;
  texture.repeat.set(1, 1);
  texture.offset.x = 0.5;
  texture.anisotropy = renderer.capabilities.getMaxAnisotropy();
  const cloth = new THREE.MeshStandardMaterial({
    map: texture,
    side: THREE.DoubleSide,
    roughness: 0.82,
  });
  function mesh(g, m, x, y, z, sx = 1, sy = 1, sz = 1) {
    const a = new THREE.Mesh(g, m);
    a.position.set(x, y, z);
    a.scale.set(sx, sy, sz);
    a.castShadow = true;
    // Floor receives cast shadows. The garment uses smooth material lighting;
    // sampling its own shadow map caused the dense diagonal "wrinkle" artifacts.
    a.receiveShadow = false;
    scene.add(a);
    return a;
  }
  mesh(new THREE.SphereGeometry(0.19, 32, 24), skin, 0, 2.91, 0, 1, 1.3, 0.93);
  mesh(new THREE.CylinderGeometry(0.085, 0.11, 0.23, 24), skin, 0, 2.625, 0);
  const dress = mesh(createDressGeometry(), cloth, 0, 0, 0);
  for (const side of [-1, 1]) {
    const arm = mesh(
      new THREE.CapsuleGeometry(0.065, 0.83, 6, 18),
      skin,
      side * 0.47,
      1.98,
      0,
    );
    arm.rotation.z = side * 0.19;
    mesh(
      new THREE.CapsuleGeometry(0.075, 0.62, 6, 18),
      skin,
      side * 0.15,
      0.38,
      0,
    );
    mesh(
      new THREE.SphereGeometry(0.1, 20, 16),
      skin,
      side * 0.15,
      0.025,
      0.08,
      1,
      0.5,
      1.8,
    );
  }
  const floor = mesh(
    new THREE.CylinderGeometry(0.85, 0.9, 0.08, 64),
    new THREE.MeshStandardMaterial({ color: 0xd8d0c2, roughness: 1 }),
    0,
    -0.07,
    0,
  );
  floor.receiveShadow = true;
  floor.castShadow = false;
  const ro = new ResizeObserver(() => {
    const w = host.clientWidth,
      h = host.clientHeight;
    if (!w || !h) return;
    renderer.setSize(w, h, false);
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
  });
  ro.observe(host);
  let frame;
  function tick() {
    controls.update();
    renderer.render(scene, camera);
    frame = requestAnimationFrame(tick);
  }
  tick();
  return {
    update(pitch = 110) {
      texture.repeat.set(110 / pitch, 110 / pitch);
      texture.needsUpdate = true;
    },
    reset() {
      camera.position.set(0, 1.3, 5.7);
      controls.target.set(0, 1.55, 0);
      controls.update();
    },
    export() {
      return renderer.domElement.toDataURL("image/png");
    },
    dispose() {
      cancelAnimationFrame(frame);
      ro.disconnect();
      controls.dispose();
      scene.traverse((o) => {
        o.geometry?.dispose();
        if (o.material)
          (Array.isArray(o.material) ? o.material : [o.material]).forEach((m) =>
            m.dispose(),
          );
      });
      texture.dispose();
      renderer.dispose();
    },
    dress,
  };
}
