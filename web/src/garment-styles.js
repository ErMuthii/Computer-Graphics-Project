const upper = [
  [0.28, 1.98],
  [0.34, 2.2],
  [0.36, 2.38],
  [0.3, 2.46],
  [0.13, 2.52],
];
export const garmentStyles = [
  {
    id: "a-line",
    label: "A-line",
    profile: [
      [0.62, 0.57],
      [0.545, 0.86],
      [0.435, 1.17],
      [0.31, 1.48],
      [0.235, 1.72],
      ...upper,
    ],
  },
  {
    id: "maxi",
    label: "Maxi",
    profile: [
      [0.59, 0.22],
      [0.54, 0.6],
      [0.45, 1.05],
      [0.31, 1.48],
      [0.235, 1.72],
      ...upper,
    ],
  },
  {
    id: "midi",
    label: "Midi",
    profile: [
      [0.53, 0.85],
      [0.46, 1.12],
      [0.33, 1.44],
      [0.235, 1.72],
      ...upper,
    ],
  },
  {
    id: "shift",
    label: "Shift",
    profile: [[0.39, 0.7], [0.38, 1.05], [0.35, 1.42], [0.3, 1.72], ...upper],
  },
  {
    id: "fit-flare",
    label: "Fit & flare",
    profile: [
      [0.74, 0.65],
      [0.64, 0.95],
      [0.43, 1.32],
      [0.235, 1.72],
      ...upper,
    ],
  },
];
export function garmentProfile(style = "a-line") {
  const match = garmentStyles.find((s) => s.id === style);
  if (!match) throw Error("Unknown dress style.");
  return match.profile;
}
