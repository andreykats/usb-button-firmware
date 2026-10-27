// ─── How-to animations: the BOOT button ─────────────────────────────────────
// Waveshare's ESP32-S3-Zero (models/esp32-s3-zero.glb, made from their STEP model
// by tools/step_to_glb.py) acting out two panels of index.html with the firmware's
// timings: holding BOOT into settings mode (Connect step 2, #howto) and pressing it
// to step to the next key (Change the key, #howtoKey). index.html imports this only
// where Web Serial is available. If WebGL or a file is unavailable the figures stay
// hidden and the written steps stand alone.
import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { RoomEnvironment } from 'three/addons/environments/RoomEnvironment.js';

const SETTINGS_HOLD_S   = 3;               // SETTINGS_HOLD_MS
const RAINBOW_PERIOD_S  = 3;               // RAINBOW_PERIOD_MS
const RAINBOW_START_POS = 4 * 256 + 160;   // RAINBOW_START_POS: purple
const PRESS_FEEDBACK_S  = 0.2;             // PRESS_FEEDBACK_MS: white after a BOOT release
const COLOR_GREEN = [0, 255, 0];           // normal state, plugged into a computer
const COLOR_WHITE = [200, 200, 200];       // press delivered
const KEY_ORDER   = 'abcdefghijklmnopqrstuvwxyz0123456789';   // nextKeyChar()
const DEFAULT_KEY = '1';
const RESTART_S   = 0.3;                   // light off while the button restarts
const PRESS_S     = 0.35;                  // hand moving onto / off BOOT
const BOB_S       = 1.8;                   // one bob of the waiting hand

// Scene sizes, in mm like the model
const PLUNGER_TRAVEL = 0.25;
const USB_C_CENTRE_Y = 1.6;                // receptacle shell centre above the PCB
// The hand is an outline drawn over the scene (the SVG in index.html, Lucide's
// "pointer" icon), turned so its finger points right at BOOT
const HAND_SIZE_MM = 14;                   // the icon's 24-unit box at the board's scale
const HAND_TIP     = [8, 4];               // icon units: the fingertip's centre, put on BOOT
const HAND_LIFT    = 3;                    // mm it backs off BOOT, away from its fingertip
const RING_COLOUR  = 0xFF8A00;             // hold-time ring: orange

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

const clamp01 = x => Math.min(Math.max(x, 0), 1);
const ease = x => x * x * (3 - 2 * x);

// ─── Scripts ────────────────────────────────────────────────────────────────
// Times in seconds from the start of each loop: the hand is on BOOT from press to
// release. led(t) is the RGB LED (null = off), ring(t) the hold-time ring's
// progress (null = hidden), keySteps(t) the keys stepped so far this loop.

// Holding BOOT into settings mode. The hold is real time; settings mode is shown
// for half a rainbow cycle (purple to yellow) at the real speed.
const holdForSettings = (() => {
  const press = 0.6;
  const restart = press + SETTINGS_HOLD_S;
  const settings = restart + RESTART_S;
  const release = settings + 0.5;
  return {
    press, release,
    duration: settings + RAINBOW_PERIOD_S / 2,
    still: press + 2.5,                    // the frame shown with reduced motion
    led(t) {
      if (t < restart) return COLOR_GREEN;
      if (t < settings) return null;
      const elapsed = (t - settings) % RAINBOW_PERIOD_S;
      return colorWheel((RAINBOW_START_POS + Math.floor(elapsed / RAINBOW_PERIOD_S * 1536)) % 1536);
    },
    ring: t => t >= press - PRESS_S && t < release ? clamp01((t - press) / SETTINGS_HOLD_S) : null,
  };
})();

// A quick press of BOOT: on release the key steps on and the LED flashes white.
// The key keeps stepping from loop to loop, as it would on the button.
const pressForNextKey = (() => {
  const press = 0.6;
  const release = press + 0.3;
  return {
    press, release,
    duration: release + 1.6,
    still: press + 0.15,
    led: t => t >= release && t < release + PRESS_FEEDBACK_S ? COLOR_WHITE : COLOR_GREEN,
    keySteps: t => t >= release ? 1 : 0,
    keyLit: t => t >= release && t < release + 0.6,   // the keycap shows the change
  };
})();

// 0 = hand off BOOT, 1 = BOOT fully down
function pressAmount(script, t) {
  if (t < script.press) return ease(clamp01((t - (script.press - PRESS_S)) / PRESS_S));
  return 1 - ease(clamp01((t - script.release) / PRESS_S));
}

// ─── Scene parts ────────────────────────────────────────────────────────────
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

function roundedRect(w, h, r) {
  const s = new THREE.Shape();
  s.moveTo(-w / 2 + r, -h / 2);
  s.lineTo(w / 2 - r, -h / 2);  s.quadraticCurveTo(w / 2, -h / 2, w / 2, -h / 2 + r);
  s.lineTo(w / 2, h / 2 - r);   s.quadraticCurveTo(w / 2, h / 2, w / 2 - r, h / 2);
  s.lineTo(-w / 2 + r, h / 2);  s.quadraticCurveTo(-w / 2, h / 2, -w / 2, h / 2 - r);
  s.lineTo(-w / 2, -h / 2 + r); s.quadraticCurveTo(-w / 2, -h / 2, -w / 2 + r, -h / 2);
  return s;
}

// The USB-C cable, plugged into the receptacle whose mouth is at z = mouthZ
function makeCable(mouthZ) {
  const cable = new THREE.Group();
  const plastic = new THREE.MeshStandardMaterial({ color: 0x2b2e34, roughness: 0.55 });
  const steel = new THREE.MeshStandardMaterial({ color: 0xc8ccd2, metalness: 1, roughness: 0.35 });
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

// A ring around BOOT that fills over SETTINGS_HOLD_S, clockwise from the top
function makeProgressRing() {
  const SEGMENTS = 96;
  const geometry = new THREE.RingGeometry(2.3, 2.95, SEGMENTS, 1, -Math.PI / 2, Math.PI * 2);
  const track = new THREE.Mesh(geometry, new THREE.MeshBasicMaterial(
    { color: RING_COLOUR, transparent: true, opacity: 0.25, side: THREE.DoubleSide, depthWrite: false }));
  const fill = new THREE.Mesh(geometry.clone(), new THREE.MeshBasicMaterial(
    { color: RING_COLOUR, side: THREE.DoubleSide }));
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

// ─── A figure: its own renderer and copy of the board, run by a script ─────
function startFigure(figure, model, script) {
  const canvas = figure.querySelector('canvas');
  figure.append(document.getElementById('howtoHand').content.cloneNode(true));
  const hand = figure.querySelector('.howto-hand');
  const keycap = figure.querySelector('.howto-key');

  const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: true });
  renderer.setPixelRatio(Math.min(devicePixelRatio, 2));

  const scene = new THREE.Scene();
  const pmrem = new THREE.PMREMGenerator(renderer);
  scene.environment = pmrem.fromScene(new RoomEnvironment(), 0.04).texture;
  scene.environmentIntensity = 0.8;
  const sun = new THREE.DirectionalLight(0xffffff, 1.8);
  sun.position.set(18, 45, -12);
  scene.add(sun);

  // Face on from straight above, USB-C at the top like Waveshare's pinout drawing
  const camera = new THREE.PerspectiveCamera(25, 16 / 9, 1, 500);
  camera.up.set(0, 0, -1);
  camera.position.set(0, 78, -1);
  camera.lookAt(0, 0, -1);
  camera.updateMatrixWorld();   // placeHand() projects before the first render

  const board = model.clone();
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
  let ring = null;
  if (script.ring) {
    ring = makeProgressRing();
    ring.position.set(bootCentre.x, 0.02, bootCentre.z);
    scene.add(ring);
  }

  // The hand's fingertip sits on BOOT; placed in pixels whenever the canvas resizes
  hand.style.transformOrigin = `${HAND_TIP[0] / 24 * 100}% ${HAND_TIP[1] / 24 * 100}%`;
  let pxPerMm = 0;
  function placeHand() {
    const project = p => {
      const v = p.clone().project(camera);
      return [(v.x + 1) / 2 * canvas.clientWidth, (1 - v.y) / 2 * canvas.clientHeight];
    };
    const plunger = new THREE.Vector3(bootCentre.x, plungerTop, bootCentre.z);
    const [x, y] = project(plunger);
    pxPerMm = (project(plunger.clone().setX(plunger.x + 10))[0] - x) / 10;
    const size = HAND_SIZE_MM * pxPerMm;
    Object.assign(hand.style, {
      width: `${size}px`, height: `${size}px`,
      left: `${x - HAND_TIP[0] / 24 * size}px`, top: `${y - HAND_TIP[1] / 24 * size}px`,
    });
  }

  const colour = new THREE.Color();
  const bootRestY = boot.position.y;
  const bobs = Math.max(1, Math.round(script.duration / BOB_S));   // whole bobs per loop
  const firstKey = KEY_ORDER.indexOf(DEFAULT_KEY);

  // t: seconds into the loop; loop: loops completed, for the key that carries on
  function pose(t, loop) {
    const press = pressAmount(script, t);
    const bob = 0.3 * Math.sin(2 * Math.PI * bobs * t / script.duration) * (1 - press);
    // Slides in from the left, then shrinks a little as it presses
    const lift = (HAND_LIFT + bob) * (1 - press) * pxPerMm;
    hand.style.transform = `translateX(${-lift}px) rotate(90deg) scale(${1 - 0.08 * press})`;
    boot.position.y = bootRestY - PLUNGER_TRAVEL * press;

    if (ring) {
      const progress = script.ring(t);
      ring.visible = progress !== null;
      if (ring.visible) ring.setProgress(progress);
    }
    if (keycap) {
      keycap.textContent = KEY_ORDER[(firstKey + loop + script.keySteps(t)) % KEY_ORDER.length];
      keycap.classList.toggle('lit', script.keyLit(t));
    }

    const rgb = script.led(t);
    if (rgb) colour.setRGB(rgb[0] / 255, rgb[1] / 255, rgb[2] / 255, THREE.SRGBColorSpace);
    else colour.setRGB(0, 0, 0);
    for (const m of ledMaterials) { m.emissive.copy(colour); m.emissiveIntensity = 1.6; }
    glow.visible = !!rgb;
    glow.material.color.copy(colour);
    ledLight.color.copy(colour);
    ledLight.intensity = rgb ? 20 : 0;
  }

  const reducedMotion = matchMedia('(prefers-reduced-motion: reduce)');
  let t = reducedMotion.matches ? script.still : 0;
  let loop = 0;
  let onScreen = false;
  let last = 0;

  function draw() {
    pose(t, loop);
    renderer.render(scene, camera);
  }

  function frame(now) {
    t += Math.min((now - last) / 1000, 0.1);
    last = now;
    if (t >= script.duration) { t -= script.duration; loop++; }
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
    if (reducedMotion.matches) { t = script.still; loop = 0; }
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
    placeHand();
    draw();
  }).observe(canvas);

  figure.hidden = false;
}

const FIGURES = [['howto', holdForSettings], ['howtoKey', pressForNextKey]];

async function startHowtos() {
  const gltf = await new GLTFLoader().loadAsync('models/esp32-s3-zero.glb');
  for (const [id, script] of FIGURES) {
    const figure = document.getElementById(id);
    try {
      startFigure(figure, gltf.scene, script);
    } catch (err) {
      console.warn(`How-to animation #${id} unavailable:`, err);
      figure.hidden = true;
    }
  }
}

startHowtos().catch(err => console.warn('How-to animations unavailable:', err));
