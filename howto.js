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
const PRESS_S    = 0.35;                   // finger moving onto / off BOOT

// Scene sizes, in mm like the model
const PLUNGER_TRAVEL = 0.25;
const USB_C_CENTRE_Y = 1.6;                // receptacle shell centre above the PCB
// The finger comes in from the left, tip to hand along FINGER_AXIS. It's slimmer than
// life so its tip stops short of the LED; it touches BOOT left of the plunger's centre.
const FINGER_AXIS   = new THREE.Vector3(-1, 0.3, 0.1).normalize();
const FINGER_RADIUS = 2.5;
const FINGER_LIFT   = 5;                   // how far it backs off BOOT, along its axis
const FINGER_TOUCH_X = -0.5;               // contact point, from the plunger's centre
const ACCENT = 0x3B3FD6;                   // --accent

const figure = document.getElementById('howto');

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

const clamp01 = x => Math.min(Math.max(x, 0), 1);
const ease = x => x * x * (3 - 2 * x);

// 0 = finger off BOOT, 1 = BOOT fully down
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

// A finger that presses BOOT. Its origin is the centre of the rounded tip; local +Y
// runs along FINGER_AXIS towards the hand and local +Z is the nail side, turned up.
function makeFinger() {
  const LENGTH = 60;
  const r = FINGER_RADIUS;
  const finger = new THREE.Group();
  const skin = new THREE.Mesh(new THREE.CapsuleGeometry(r, LENGTH, 12, 32),
    new THREE.MeshStandardMaterial({ color: 0xdba184, roughness: 0.6 }));
  skin.position.y = LENGTH / 2;
  const nail = new THREE.Mesh(new THREE.SphereGeometry(1, 32, 16),
    new THREE.MeshStandardMaterial({ color: 0xeebcaa, roughness: 0.3 }));
  nail.scale.set(0.55 * r, 0.8 * r, 0.1 * r);
  nail.position.set(0, 0.6 * r, r - 0.05);
  finger.add(skin, nail);

  const along = FINGER_AXIS.clone();
  const up = new THREE.Vector3(0, 1, 0);
  const nailSide = up.addScaledVector(along, -up.dot(along)).normalize();
  const side = new THREE.Vector3().crossVectors(along, nailSide);
  finger.quaternion.setFromRotationMatrix(new THREE.Matrix4().makeBasis(side, along, nailSide));
  finger.traverse(o => { o.castShadow = true; });
  return finger;
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
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.PCFSoftShadowMap;

  const scene = new THREE.Scene();
  const pmrem = new THREE.PMREMGenerator(renderer);
  scene.environment = pmrem.fromScene(new RoomEnvironment(), 0.04).texture;
  scene.environmentIntensity = 0.8;
  // From the top right, so the finger's shadow falls away from the LED
  const sun = new THREE.DirectionalLight(0xffffff, 1.8);
  sun.position.set(18, 45, -12);
  sun.castShadow = true;
  sun.shadow.mapSize.set(2048, 2048);
  Object.assign(sun.shadow.camera, { left: -24, right: 24, top: 24, bottom: -24, near: 1, far: 120 });
  sun.shadow.normalBias = 0.03;
  scene.add(sun);

  // Face on from straight above, USB-C at the top like Waveshare's pinout drawing
  const camera = new THREE.PerspectiveCamera(25, 16 / 9, 1, 500);
  camera.up.set(0, 0, -1);
  camera.position.set(0, 78, -1);
  camera.lookAt(0, 0, -1);

  const gltf = await new GLTFLoader().loadAsync('models/esp32-s3-zero.glb');
  const board = gltf.scene;
  board.traverse(o => { o.castShadow = o.receiveShadow = true; });
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

  const cable = makeCable(boardBox.min.z);
  cable.traverse(o => { o.castShadow = true; });
  scene.add(cable);
  const finger = makeFinger();
  // Where the tip's centre is when it just touches the plunger
  const touch = new THREE.Vector3(bootCentre.x + FINGER_TOUCH_X, plungerTop + FINGER_RADIUS, bootCentre.z);
  const ring = makeProgressRing();
  ring.position.set(bootCentre.x, 0.02, bootCentre.z);
  scene.add(finger, ring);

  const colour = new THREE.Color();
  const bootRestY = boot.position.y;

  function pose(t) {
    const press = pressAmount(t);
    const bob = 0.3 * Math.sin(2 * Math.PI * 6 * t / T_END) * (1 - press);
    // Backs off along its own axis, presses straight down; BOOT moves once touched
    finger.position.copy(touch).addScaledVector(FINGER_AXIS, (FINGER_LIFT + bob) * (1 - press));
    finger.position.y -= PLUNGER_TRAVEL * press;
    const gap = finger.position.y - touch.y;
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
  }

  const reducedMotion = matchMedia('(prefers-reduced-motion: reduce)');
  let t = reducedMotion.matches ? T_STILL : 0;
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

  // Runs only while it's visible and motion is welcome; otherwise one frame on changes
  function updateLoop() {
    const run = !reducedMotion.matches && onScreen && !document.hidden;
    last = performance.now();
    renderer.setAnimationLoop(run ? frame : null);
    if (!run) draw();
  }

  reducedMotion.addEventListener('change', () => {
    if (reducedMotion.matches) t = T_STILL;
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
