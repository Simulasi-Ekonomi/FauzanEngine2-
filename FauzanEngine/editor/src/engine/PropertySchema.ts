export type PropertyKind = 'number' | 'boolean' | 'string';

export interface PropertyDefinition {
  key: string;
  label: string;
  kind: PropertyKind;
  defaultValue: string | number | boolean;
  min?: number;
  max?: number;
  step?: number;
  readOnly?: boolean;
}

const commonDefinitions: PropertyDefinition[] = [
  { key: 'enabled', label: 'Enabled', kind: 'boolean', defaultValue: true },
  { key: 'intensity', label: 'Intensity', kind: 'number', defaultValue: 1, min: 0, max: 1000, step: 0.1 },
  { key: 'castShadows', label: 'Cast Shadows', kind: 'boolean', defaultValue: true },
  { key: 'blendWeight', label: 'Blend Weight', kind: 'number', defaultValue: 1, min: 0, max: 1, step: 0.01 },
  { key: 'assetId', label: 'Asset', kind: 'string', defaultValue: '', readOnly: true },
];

export function getPropertyDefinition(componentType: string, key: string, value: unknown): PropertyDefinition {
  const known = commonDefinitions.find((definition) => definition.key === key);
  if (known) return known;
  const kind: PropertyKind = typeof value === 'number' ? 'number' : typeof value === 'boolean' ? 'boolean' : 'string';
  return { key, label: key.replace(/([A-Z])/g, ' $1').replace(/^./, (character) => character.toUpperCase()), kind, defaultValue: kind === 'number' ? 0 : kind === 'boolean' ? true : '', ...(componentType === 'LightComponent' && key === 'range' ? { min: 0, max: 10000, step: 0.1 } : {}) };
}

export function coerceProperty(definition: PropertyDefinition, raw: unknown): string | number | boolean {
  if (definition.kind === 'boolean') return Boolean(raw);
  if (definition.kind === 'number') {
    const parsed = typeof raw === 'number' ? raw : Number(raw);
    if (!Number.isFinite(parsed)) return definition.defaultValue as number;
    return Math.min(definition.max ?? parsed, Math.max(definition.min ?? parsed, parsed));
  }
  return String(raw ?? '');
}
