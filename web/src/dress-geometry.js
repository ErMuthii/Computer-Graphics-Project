import { LatheGeometry, Vector2, Vector3 } from "three";

// Radius and height in mannequin world units, ordered from hem to neckline.
const profile = [
  [0.62, 0.57],
  [0.545, 0.86],
  [0.435, 1.17],
  [0.31, 1.48],
  [0.235, 1.72],
  [0.28, 1.98],
  [0.34, 2.2],
  [0.36, 2.38],
  [0.3, 2.46],
  [0.13, 2.52],
];
export const PRINT_PERIOD = 0.22;

// Shape-preserving cubic interpolation avoids bulges/overshoot at the waist
// while replacing the original nine straight profile segments with a smooth curve.
export function sampleDressProfile(rows = 96) {
  const h = profile.slice(1).map((p, i) => p[1] - profile[i][1]);
  const slopes = h.map((d, i) => (profile[i + 1][0] - profile[i][0]) / d);
  const tangents = profile.map((_, i) => {
    if (i === 0) return slopes[0];
    if (i === profile.length - 1) return slopes.at(-1);
    if (slopes[i - 1] * slopes[i] <= 0) return 0;
    const a = 2 * h[i] + h[i - 1],
      b = h[i] + 2 * h[i - 1];
    return (a + b) / (a / slopes[i - 1] + b / slopes[i]);
  });
  const bottom = profile[0][1],
    top = profile.at(-1)[1];
  return Array.from({ length: rows + 1 }, (_, j) => {
    const y = bottom + ((top - bottom) * j) / rows;
    let i = 0;
    while (i < profile.length - 2 && y > profile[i + 1][1]) i++;
    const t = (y - profile[i][1]) / h[i],
      t2 = t * t,
      t3 = t2 * t;
    const radius =
      (2 * t3 - 3 * t2 + 1) * profile[i][0] +
      (t3 - 2 * t2 + t) * h[i] * tangents[i] +
      (-2 * t3 + 3 * t2) * profile[i + 1][0] +
      (t3 - t2) * h[i] * tangents[i + 1];
    return new Vector2(radius, y);
  });
}

export function createDressGeometry({ segments = 96, rows = 96 } = {}) {
  const points = sampleDressProfile(rows);
  // Put the construction seam at the back, away from the default front view.
  const geometry = new LatheGeometry(points, segments, Math.PI);
  const position = geometry.attributes.position,
    uv = geometry.attributes.uv;
  for (let i = 0; i <= segments; i++)
    for (let j = 0; j <= rows; j++) {
      const n = i * (rows + 1) + j,
        theta = Math.PI + (i * 2 * Math.PI) / segments,
        y = position.getY(n);
      const skirt = Math.max(0, Math.min(1, (1.72 - y) / (1.72 - 0.57)));
      const folds = 1 + 0.018 * skirt * skirt * Math.cos(theta * 12);
      position.setXYZ(
        n,
        position.getX(n) * folds,
        y,
        position.getZ(n) * 0.63 * folds,
      );
    }
  // UVs measure physical height and arc length instead of profile-row number.
  // A motif keeps its scale across unevenly spaced control levels and no longer
  // repeats a cropped 720x480 preview. There is one deliberate back garment seam.
  for (let j = 0; j <= rows; j++) {
    const arc = [0];
    for (let i = 1; i <= segments; i++) {
      const a = (i - 1) * (rows + 1) + j,
        b = i * (rows + 1) + j;
      arc.push(
        arc.at(-1) +
          Math.hypot(
            position.getX(b) - position.getX(a),
            position.getZ(b) - position.getZ(a),
          ),
      );
    }
    const front = arc[segments / 2];
    for (let i = 0; i <= segments; i++) {
      const n = i * (rows + 1) + j;
      uv.setXY(
        n,
        (arc[i] - front) / PRINT_PERIOD,
        (position.getY(n) - points[0].y) / PRINT_PERIOD,
      );
    }
  }
  geometry.computeVertexNormals();
  // The duplicated back-seam vertices share a normal, preventing a lighting line.
  const normals = geometry.attributes.normal;
  for (let j = 0; j <= rows; j++) {
    const last = segments * (rows + 1) + j;
    const n = new Vector3(
      normals.getX(j) + normals.getX(last),
      normals.getY(j) + normals.getY(last),
      normals.getZ(j) + normals.getZ(last),
    ).normalize();
    normals.setXYZ(j, n.x, n.y, n.z);
    normals.setXYZ(last, n.x, n.y, n.z);
  }
  geometry.computeBoundingSphere();
  return geometry;
}
