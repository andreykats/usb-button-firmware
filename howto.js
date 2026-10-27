// ─── How-to animation: holding BOOT into settings mode ──────────────────────
// Waveshare's ESP32-S3-Zero (models/esp32-s3-zero.glb, made from their STEP model
// by tools/step_to_glb.py) acting out Connect step 2 of index.html with the
// firmware's timings. index.html imports this only when the Connect panel is shown.
// If WebGL or a file is unavailable the figure stays hidden and the written steps
// stand alone.
import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { RoomEnvironment } from 'three/addons/environments/RoomEnvironment.js';

const SETTINGS_HOLD_S   = 3;               // SETTINGS_HOLD_MS
const RAINBOW_PERIOD_S  = 3;               // RAINBOW_PERIOD_MS
const RAINBOW_START_POS = 4 * 256 + 160;   // RAINBOW_START_POS: purple
const RESTART_S         = 0.3;             // light off while the button restarts

// Timeline, in seconds from the start of each loop
const T_PRESS    = 1.4;
const T_RESTART  = T_PRESS + SETTINGS_HOLD_S;
const T_SETTINGS = T_RESTART + RESTART_S;
const T_RELEASE  = T_SETTINGS + 0.8;
const T_END      = T_SETTINGS + 2 * RAINBOW_PERIOD_S;
const T_STILL    = T_PRESS + 2.5;          // the frame shown with reduced motion
const PRESS_S    = 0.35;                   // pointer moving onto / off BOOT

// Scene sizes, in mm like the model
const PLUNGER_TRAVEL = 0.25;
const POINTER_REST   = 3.2;                // pointer tip height above the plunger
const USB_C_CENTRE_Y = 1.6;                // receptacle shell centre above the PCB
// The pointer comes in from the left, across the view and clear of the plug
const POINTER_AXIS = new THREE.Vector3(-1, 1.1, 0.25).normalize();
const POINTER_LENGTH = 9;                  // cone + shaft: the BOOT tag sits at its tail
const ACCENT = 0x3B3FD6;                   // --accent

const figure = document.getElementById('howto');
const captionEl = figure.querySelector('figcaption');
const tagEl = figure.querySelector('.howto-tag');
const pauseBtn = figure.querySelector('button');

// The firmware's colorWheel(): pos 0..1535, six 256-step ramps
function colorWheel(pos) {
  const f = pos & 0xFF;
  switch (pos >> 8) {
    case 0:  return [255, f, 0];
    case 1:  return [255 - f, 255, 0];
    case 2:  return [0, 255, f];
    case 3:  return [0, 255 - f, 255];
    case 4:  return [f, 0, 255];
    default: return [255, 0, 255 - f];
  }
}

// The RGB LED at time t (null = off): green while plugged in (COLOR_GREEN), off
// through the restart, then settings mode's rainbow from purple
function ledColour(t) {
  if (t < T_RESTART) return [0, 255, 0];
  if (t < T_SETTINGS) return null;
  const elapsed = (t - T_SETTINGS) % RAINBOW_PERIOD_S;
  return colorWheel((RAINBOW_START_POS + Math.floor(elapsed / RAINBOW_PERIOD_S * 1536)) % 1536);
}

function caption(t) {
  if (t < T_PRESS) return 'Plugged in: the light is green';
  if (t < T_RESTART) return `Hold BOOT · ${Math.floor(t - T_PRESS) + 1} s`;
  if (t < T_SETTINGS) return 'The button restarts…';
  return 'Settings mode: the light cycles through colours';
}

const clamp01 = x => Math.min(Math.max(x, 0), 1);
const ease = x => x * x * (3 - 2 * x);

// 0 = pointer up, 1 = BOOT fully down
function pressAmount(t) {
  if (t < T_PRESS) return ease(clamp01((t - (T_PRESS - PRESS_S)) / PRESS_S));
  return 1 - ease(clamp01((t - T_RELEASE) / PRESS_S));
}

function glowTexture() {
  const c = document.createElement('canvas');
  c.width = c.height = 64;
  const g = c.getContext('2d');
  const grad = g.createRadialGradient(32, 32, 0, 32, 32, 32);
  grad.addColorStop(0, 'rgba(255,255,255,1)');
  grad.addColorStop(0.25, 'rgba(255,255,255,.45)');
  grad.addColorStop(1, 'rgba(255,255,255,0)');
  g.fillStyle = grad;
  g.fillRect(0, 0, 64, 64);
  return new THREE.CanvasTexture(c);
}

// The USB-C cable, plugged into the receptacle whose mouth is at z = mouthZ
function makeCable(mouthZ) {
  const cable = new THREE.Group();
  const plastic = new THREE.MeshStandardMaterial({ color: 0x2b2e34, roughness: 0.55 });
  const steel = new THREE.MeshStandardMaterial({ color: 0xc8ccd2, metalness: 1, roughness: 0.35 });
  const roundedRect = (w, h, r) => {
    const s = new THREE.Shape();
    s.moveTo(-w / 2 + r, -h / 2);
    s.lineTo(w / 2 - r, -h / 2);  s.quadraticCurveTo(w / 2, -h / 2, w / 2, -h / 2 + r);
    s.lineTo(w / 2, h / 2 - r);   s.quadraticCurveTo(w / 2, h / 2, w / 2 - r, h / 2);
    s.lineTo(-w / 2 + r, h / 2);  s.quadraticCurveTo(-w / 2, h / 2, -w / 2, h / 2 - r);
    s.lineTo(-w / 2, -h / 2 + r); s.quadraticCurveTo(-w / 2, -h / 2, -w / 2 + r, -h / 2);
    return s;
  };
  const neck = new THREE.Mesh(new THREE.ExtrudeGeometry(roundedRect(8.3, 2.5, 1.2),
    { depth: 1.2, bevelEnabled: false }), steel);
  neck.position.set(0, USB_C_CENTRE_Y, mouthZ - 1.0);
  const body = new THREE.Mesh(new THREE.ExtrudeGeometry(roundedRect(11, 5.6, 2.2),
    { depth: 15, bevelThickness: 0.6, bevelSize: 0.6, bevelSegments: 3, curveSegments: 8 }), plastic);
  body.position.set(0, USB_C_CENTRE_Y, mouthZ - 1.6 - 15);
  const lead = new THREE.Mesh(new THREE.CylinderGeometry(1.8, 1.8, 80, 24), plastic);
  lead.rotation.x = Math.PI / 2;
  lead.position.set(0, USB_C_CENTRE_Y, mouthZ - 16 - 40);
  cable.add(neck, body, lead);
  return cable;
}

// A pointer that presses BOOT: its tip is the group's origin
function makePointer() {
  const pointer = new THREE.Group();
  const material = new THREE.MeshStandardMaterial({ color: ACCENT, roughness: 0.4 });
  const head = new THREE.Mesh(new THREE.ConeGeometry(1.4, 3, 32), material);
  head.rotation.x = Math.PI;
  head.position.y = 1.5;
  const shaft = new THREE.Mesh(new THREE.CylinderGeometry(0.6, 0.6, 6, 24), material);
  shaft.position.y = 3 + 3;
  pointer.add(head, shaft);
  return pointer;
}

// A ring around BOOT that fills over SETTINGS_HOLD_S, clockwise from the top
function makeProgressRing() {
  const SEGMENTS = 96;
  const geometry = new THREE.RingGeometry(2.3, 2.95, SEGMENTS, 1, -Math.PI / 2, Math.PI * 2);
  const track = new THREE.Mesh(geometry, new THREE.MeshBasicMaterial(
    { color: ACCENT, transparent: true, opacity: 0.25, side: THREE.DoubleSide, depthWrite: false }));
  const fill = new THREE.Mesh(geometry.clone(), new THREE.MeshBasicMaterial(
    { color: ACCENT, side: THREE.DoubleSide }));
  const ring = new THREE.Group();
  for (const mesh of [track, fill]) {
    mesh.rotation.x = Math.PI / 2;   // flat on the board; θ then runs clockwise seen from above
    ring.add(mesh);
  }
  fill.position.y = 0.01;
  // RingGeometry lists its segments in θ order, 6 indices each
  ring.setProgress = p => fill.geometry.setDrawRange(0, 6 * Math.round(p * SEGMENTS));
  return ring;
}

async function startHowto() {
  const canvas = figure.querySelector('canvas');
  const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: true });
  renderer.setPixelRatio(Math.min(devicePixelRatio, 2));

  const scene = new THREE.Scene();
  const pmrem = new THREE.PMREMGenerator(renderer);
  scene.environment = pmrem.fromScene(new RoomEnvironment(), 0.04).texture;
  scene.environmentIntensity = 0.8;
  const sun = new THREE.DirectionalLight(0xffffff, 1.8);
  sun.position.set(-20, 40, 25);
  scene.add(sun);

  const camera = new THREE.PerspectiveCamera(25, 16 / 9, 1, 500);
  camera.position.set(-16, 46, 50);
  camera.lookAt(0, 0, 0.5);
  camera.updateMatrixWorld();   // pose() projects the tag before the first render

  const gltf = await new GLTFLoader().loadAsync('models/esp32-s3-zero.glb');
  const board = gltf.scene;
  scene.add(board);
  const boot = board.getObjectByName('boot');
  const led = board.getObjectByName('led');
  if (!boot || !led) throw new Error('model lacks the boot or led node');

  const bootBox = new THREE.Box3().setFromObject(boot);
  const bootCentre = bootBox.getCenter(new THREE.Vector3());
  const plungerTop = bootBox.max.y;
  const ledBox = new THREE.Box3().setFromObject(led);
  const ledCentre = ledBox.getCenter(new THREE.Vector3());
  const boardBox = new THREE.Box3().setFromObject(board);

  // The LED's own material, so its colour can change without touching others
  const ledMaterials = [];
  led.traverse(o => {
    if (o.isMesh) {
      o.material = o.material.clone();
      ledMaterials.push(o.material);
    }
  });
  const glow = new THREE.Sprite(new THREE.SpriteMaterial({
    map: glowTexture(), blending: THREE.AdditiveBlending, depthWrite: false, transparent: true }));
  glow.position.set(ledCentre.x, ledBox.max.y + 0.4, ledCentre.z);
  glow.scale.setScalar(6);
  const ledLight = new THREE.PointLight(0xffffff, 0, 16, 2);
  ledLight.position.set(ledCentre.x, ledBox.max.y + 1.5, ledCentre.z);
  scene.add(glow, ledLight);

  scene.add(makeCable(boardBox.min.z));
  const pointer = makePointer();
  pointer.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), POINTER_AXIS);
  const ring = makeProgressRing();
  ring.position.set(bootCentre.x, 0.02, bootCentre.z);
  scene.add(pointer, ring);

  const colour = new THREE.Color();
  const bootRestY = boot.position.y;
  const tagAnchor = new THREE.Vector3();
  let shownCaption = '';

  function pose(t) {
    const press = pressAmount(t);
    const bob = 0.3 * Math.sin(2 * Math.PI * 6 * t / T_END) * (1 - press);
    // Distance of the tip from the plunger's top, along the pointer
    const gap = POINTER_REST + bob - press * (POINTER_REST + PLUNGER_TRAVEL);
    pointer.position.copy(bootCentre).setY(plungerTop).addScaledVector(POINTER_AXIS, gap);
    boot.position.y = bootRestY + Math.max(-PLUNGER_TRAVEL, Math.min(0, gap));

    ring.visible = t >= T_PRESS - PRESS_S && t < T_RELEASE;
    ring.setProgress(clamp01((t - T_PRESS) / SETTINGS_HOLD_S));

    const rgb = ledColour(t);
    if (rgb) colour.setRGB(rgb[0] / 255, rgb[1] / 255, rgb[2] / 255, THREE.SRGBColorSpace);
    else colour.setRGB(0, 0, 0);
    for (const m of ledMaterials) { m.emissive.copy(colour); m.emissiveIntensity = 1.6; }
    glow.visible = !!rgb;
    glow.material.color.copy(colour);
    ledLight.color.copy(colour);
    ledLight.intensity = rgb ? 20 : 0;

    const text = caption(t);
    if (text !== shownCaption) captionEl.textContent = shownCaption = text;

    // The BOOT tag rides on the pointer's tail
    tagAnchor.copy(pointer.position).addScaledVector(POINTER_AXIS, POINTER_LENGTH).project(camera);
    tagEl.style.left = `${(tagAnchor.x + 1) / 2 * 100}%`;
    tagEl.style.top = `${(1 - tagAnchor.y) / 2 * 100}%`;
  }

  const reducedMotion = matchMedia('(prefers-reduced-motion: reduce)');
  let t = reducedMotion.matches ? T_STILL : 0;
  let playing = !reducedMotion.matches;
  let onScreen = false;
  let last = 0;

  function draw() {
    pose(t);
    renderer.render(scene, camera);
  }

  function frame(now) {
    t = (t + Math.min((now - last) / 1000, 0.1)) % T_END;
    last = now;
    draw();
  }

  // Runs only while it's visible and playing; otherwise one frame on changes
  function updateLoop() {
    const run = playing && onScreen && !document.hidden;
    last = performance.now();
    renderer.setAnimationLoop(run ? frame : null);
    if (!run) draw();
    pauseBtn.textContent = playing ? 'Pause' : 'Play';
  }

  pauseBtn.addEventListener('click', () => { playing = !playing; updateLoop(); });
  reducedMotion.addEventListener('change', () => {
    playing = !reducedMotion.matches;
    if (!playing) t = T_STILL;
    updateLoop();
  });
  document.addEventListener('visibilitychange', updateLoop);
  new IntersectionObserver(([entry]) => { onScreen = entry.isIntersecting; updateLoop(); })
    .observe(figure);
  new ResizeObserver(() => {
    const { clientWidth: w, clientHeight: h } = canvas;
    if (!w || !h) return;
    renderer.setSize(w, h, false);
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
    draw();
  }).observe(canvas);

  figure.hidden = false;
}

startHowto().catch(err => {
  console.warn('Settings-mode animation unavailable:', err);
  figure.hidden = true;
});
