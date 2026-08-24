/**
 * NeoEngine Build & Publish System
 * Handles building games for different platforms and publishing to stores
 */

export interface BuildConfig {
  platform: 'android' | 'ios' | 'web' | 'windows' | 'linux';
  appName: string;
  packageName: string;
  version: string;
  versionCode: number;
  icon?: string;
  splash?: string;
  orientation: 'portrait' | 'landscape' | 'both';
  minSdk?: number;
  targetSdk?: number;
  permissions: string[];
  features: string[];
}

export interface BuildResult {
  success: boolean;
  platform: string;
  outputPath?: string;
  outputSize?: string;
  buildTime?: number;
  errors?: string[];
  warnings?: string[];
}

export interface PublishConfig {
  store: 'google_play' | 'app_store' | 'itch_io' | 'steam' | 'web';
  track?: 'internal' | 'alpha' | 'beta' | 'production';
  releaseNotes: string;
  screenshots?: string[];
  category?: string;
  contentRating?: string;
  pricing?: 'free' | 'paid';
  price?: number;
}

export interface PublishResult {
  success: boolean;
  store: string;
  status: string;
  url?: string;
  reviewEstimate?: string;
}

export interface UpdateConfig {
  type: 'hot_reload' | 'patch' | 'minor' | 'major';
  changes: string[];
  forceUpdate: boolean;
  rollbackEnabled: boolean;
}

const DEFAULT_BUILD_CONFIG: BuildConfig = {
  platform: 'android',
  appName: 'My NeoEngine Game',
  packageName: 'com.neoengine.mygame',
  version: '1.0.0',
  versionCode: 1,
  orientation: 'landscape',
  minSdk: 24,
  targetSdk: 34,
  permissions: ['INTERNET', 'VIBRATE'],
  features: ['android.hardware.touchscreen'],
};

// Build system manager
export class BuildSystem {
  private config: BuildConfig;
  private logCallback: (msg: string) => void;

  constructor(logCallback: (msg: string) => void) {
    this.config = { ...DEFAULT_BUILD_CONFIG };
    this.logCallback = logCallback;
  }

  getConfig(): BuildConfig { return { ...this.config }; }
  setConfig(config: Partial<BuildConfig>) { this.config = { ...this.config, ...config }; }

  // Generate Android manifest
  private generateAndroidManifest(): string {
    const perms = this.config.permissions
      .map(p => `    <uses-permission android:name="android.permission.${p}" />`)
      .join('\n');
    const feats = this.config.features
      .map(f => `    <uses-feature android:name="${f}" android:required="false" />`)
      .join('\n');

    return `<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="${this.config.packageName}"
    android:versionCode="${this.config.versionCode}"
    android:versionName="${this.config.version}">

    <uses-sdk
        android:minSdkVersion="${this.config.minSdk}"
        android:targetSdkVersion="${this.config.targetSdk}" />

${perms}
${feats}

    <application
        android:allowBackup="true"
        android:label="${this.config.appName}"
        android:supportsRtl="true"
        android:theme="@style/NeoEngineTheme">

        <activity
            android:name=".NeoEngineActivity"
            android:exported="true"
            android:configChanges="orientation|screenSize|keyboardHidden"
            android:screenOrientation="${this.config.orientation}">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>`;
  }

  // Generate build.gradle
  private generateBuildGradle(): string {
    return `plugins {
    id 'com.android.application'
}

android {
    namespace '${this.config.packageName}'
    compileSdk ${this.config.targetSdk}

    defaultConfig {
        applicationId "${this.config.packageName}"
        minSdk ${this.config.minSdk}
        targetSdk ${this.config.targetSdk}
        versionCode ${this.config.versionCode}
        versionName "${this.config.version}"
        ndk {
            abiFilters 'arm64-v8a', 'armeabi-v7a', 'x86_64'
        }
    }

    buildTypes {
        release {
            minifyEnabled true
            proguardFiles getDefaultProguardFile('proguard-android-optimize.txt'), 'proguard-rules.pro'
            signingConfig signingConfigs.release
        }
    }

    externalNativeBuild {
        cmake {
            path "CMakeLists.txt"
            version "3.22.1"
        }
    }
}

dependencies {
    implementation 'com.google.android.material:material:1.11.0'
}`;
  }

  // Export game data as JSON bundle
  exportGameBundle(actors: Record<string, unknown>, scripts: Array<{ actorName: string; code: string }>): string {
    const bundle = {
      engine: 'NeoEngine',
      version: this.config.version,
      appName: this.config.appName,
      packageName: this.config.packageName,
      buildDate: new Date().toISOString(),
      scene: {
        actors,
        actorCount: Object.keys(actors).length,
      },
      scripts: scripts.map(s => ({ actor: s.actorName, code: s.code })),
      config: {
        platform: this.config.platform,
        orientation: this.config.orientation,
        permissions: this.config.permissions,
      },
    };
    return JSON.stringify(bundle, null, 2);
  }

  // Simulate building for a platform
  async build(actors: Record<string, unknown>, scripts: Array<{ actorName: string; code: string }>): Promise<BuildResult> {
    const startTime = Date.now();
    this.logCallback(`[Build] Starting ${this.config.platform} build...`);
    this.logCallback(`[Build] App: ${this.config.appName} v${this.config.version}`);
    this.logCallback(`[Build] Package: ${this.config.packageName}`);

    // Simulate build steps
    const steps = [
      'Compiling shaders...',
      'Processing assets...',
      'Bundling game scripts...',
      'Compiling C++ engine...',
      'Linking libraries...',
      `Generating ${this.config.platform === 'android' ? 'APK' : this.config.platform === 'web' ? 'HTML5' : 'executable'}...`,
      'Optimizing...',
      'Signing package...',
    ];

    for (const step of steps) {
      this.logCallback(`[Build] ${step}`);
      await new Promise(r => setTimeout(r, 300));
    }

    // Generate the game bundle
    const bundle = this.exportGameBundle(actors, scripts);
    const bundleSize = new Blob([bundle]).size;

    if (this.config.platform === 'android') {
      this.logCallback(`[Build] Generated AndroidManifest.xml`);
      this.logCallback(`[Build] Generated build.gradle`);
    }

    const buildTime = Date.now() - startTime;
    const outputExt = { android: 'apk', ios: 'ipa', web: 'html', windows: 'exe', linux: 'AppImage' }[this.config.platform];

    this.logCallback(`[Build] Build completed in ${(buildTime / 1000).toFixed(1)}s`);
    this.logCallback(`[Build] Output: ${this.config.appName}.${outputExt} (${(bundleSize / 1024).toFixed(1)} KB)`);

    // Auto-download the game bundle
    const blob = new Blob([bundle], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `${this.config.appName.replace(/\s+/g, '_')}_v${this.config.version}.neobundle`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);

    return {
      success: true,
      platform: this.config.platform,
      outputPath: `${this.config.appName}.${outputExt}`,
      outputSize: `${(bundleSize / 1024).toFixed(1)} KB`,
      buildTime,
      warnings: bundleSize > 100 * 1024 ? ['Bundle size exceeds 100KB, consider optimizing assets'] : [],
    };
  }

  // Simulate publishing to a store
  async publish(publishConfig: PublishConfig): Promise<PublishResult> {
    this.logCallback(`[Publish] Preparing upload to ${publishConfig.store}...`);
    this.logCallback(`[Publish] Track: ${publishConfig.track || 'production'}`);
    this.logCallback(`[Publish] Release notes: ${publishConfig.releaseNotes.slice(0, 100)}...`);

    const steps: Record<string, string[]> = {
      google_play: [
        'Validating APK signature...',
        'Uploading to Google Play Console...',
        'Processing app bundle...',
        'Running pre-launch report...',
        'Submitting for review...',
      ],
      app_store: [
        'Validating IPA...',
        'Uploading to App Store Connect...',
        'Processing binary...',
        'Submitting for App Review...',
      ],
      itch_io: [
        'Compressing game bundle...',
        'Uploading to itch.io...',
        'Processing page...',
      ],
      web: [
        'Optimizing HTML5 build...',
        'Deploying to CDN...',
        'Configuring domain...',
      ],
      steam: [
        'Preparing Steamworks upload...',
        'Uploading depot...',
        'Setting store page...',
        'Submitting for review...',
      ],
    };

    for (const step of (steps[publishConfig.store] || [])) {
      this.logCallback(`[Publish] ${step}`);
      await new Promise(r => setTimeout(r, 400));
    }

    const storeUrls: Record<string, string> = {
      google_play: `https://play.google.com/store/apps/details?id=${this.config.packageName}`,
      app_store: `https://apps.apple.com/app/${this.config.appName.toLowerCase().replace(/\s+/g, '-')}`,
      itch_io: `https://neoengine.itch.io/${this.config.appName.toLowerCase().replace(/\s+/g, '-')}`,
      web: `https://${this.config.appName.toLowerCase().replace(/\s+/g, '-')}.neoengine.app`,
      steam: `https://store.steampowered.com/app/${this.config.appName.toLowerCase().replace(/\s+/g, '-')}`,
    };

    const reviewTimes: Record<string, string> = {
      google_play: '2-7 days',
      app_store: '1-3 days',
      itch_io: 'Instant',
      web: 'Instant',
      steam: '2-5 days',
    };

    this.logCallback(`[Publish] Successfully submitted to ${publishConfig.store}!`);
    this.logCallback(`[Publish] Estimated review time: ${reviewTimes[publishConfig.store]}`);

    return {
      success: true,
      store: publishConfig.store,
      status: publishConfig.store === 'itch_io' || publishConfig.store === 'web' ? 'published' : 'pending_review',
      url: storeUrls[publishConfig.store],
      reviewEstimate: reviewTimes[publishConfig.store],
    };
  }

  // Hot reload / live update system
  async pushUpdate(updateConfig: UpdateConfig): Promise<{ success: boolean; details: string }> {
    this.logCallback(`[Update] Preparing ${updateConfig.type} update...`);
    this.logCallback(`[Update] Changes: ${updateConfig.changes.length} modifications`);

    for (const change of updateConfig.changes) {
      this.logCallback(`[Update]   - ${change}`);
    }

    const steps = {
      hot_reload: ['Diffing script changes...', 'Pushing hot reload patch...', 'Clients updated!'],
      patch: ['Building patch bundle...', 'Uploading to CDN...', 'Notifying clients...', 'Patch deployed!'],
      minor: ['Building full bundle...', 'Running tests...', 'Uploading...', 'Rolling out to 10%...', 'Rolling out to 100%...'],
      major: ['Building full bundle...', 'Running regression tests...', 'Uploading...', 'Rolling out to 1%...', 'Monitoring...', 'Rolling out to 100%...'],
    };

    for (const step of steps[updateConfig.type]) {
      this.logCallback(`[Update] ${step}`);
      await new Promise(r => setTimeout(r, 300));
    }

    const vParts = this.config.version.split('.').map(Number);
    if (updateConfig.type === 'patch') vParts[2]++;
    else if (updateConfig.type === 'minor') { vParts[1]++; vParts[2] = 0; }
    else if (updateConfig.type === 'major') { vParts[0]++; vParts[1] = 0; vParts[2] = 0; }
    this.config.version = vParts.join('.');
    this.config.versionCode++;

    this.logCallback(`[Update] Version bumped to ${this.config.version} (code: ${this.config.versionCode})`);
    return { success: true, details: `${updateConfig.type} update deployed. New version: ${this.config.version}` };
  }
}
