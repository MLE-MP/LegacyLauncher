const { ipcRenderer, shell } = require('electron');
const fs = require('fs');
const path = require('path');
const https = require('https');
const extractZip = require('extract-zip');
const childProcess = require('child_process');

const DEFAULT_REPO = "MLE-MP/MinecraftConsoles";
const DEFAULT_EXEC = "Minecraft.Client.exe";
const TARGET_FILE = "LCEWindows64.zip";

let releasesData = [];
let currentReleaseIndex = 0;
let isProcessing = false;
let isGameRunning = false;


const Store = {
    async get(key, defaultValue) {
        const val = await ipcRenderer.invoke('store-get', key);
        return val !== undefined ? val : defaultValue;
    },
    async set(key, value) {
        return await ipcRenderer.invoke('store-set', key, value);
    }
};

window.onload = async () => {
    document.getElementById('repo-input').value = await Store.get('legacy_repo', DEFAULT_REPO);
    document.getElementById('exec-input').value = await Store.get('legacy_exec_path', DEFAULT_EXEC);
    document.getElementById('username-input').value = await Store.get('legacy_username', "");
    document.getElementById('ip-input').value = await Store.get('legacy_ip', "");
    document.getElementById('port-input').value = await Store.get('legacy_port', "");
    document.getElementById('server-checkbox').checked = await Store.get('legacy_is_server', false);
    
    if (process.platform === 'linux') {
        document.getElementById('compat-option-container').style.display = 'block';
        scanCompatibilityLayers();
    }

    ipcRenderer.on('window-is-maximized', (event, isMaximized) => {
        document.getElementById('maximize-btn').textContent = isMaximized ? '❐' : '▢';
    });

    fetchGitHubData();
};

async function scanCompatibilityLayers() {
    const select = document.getElementById('compat-select');
    const savedValue = await Store.get('legacy_compat_layer', 'direct');
    
    const layers = [
        { name: 'Default (Direct)', cmd: 'direct' },
        { name: 'Wine64', cmd: 'wine64' },
        { name: 'Wine', cmd: 'wine' }
    ];

    const homeDir = require('os').homedir();
    const steamPaths = [
        path.join(homeDir, '.steam', 'steam', 'steamapps', 'common'),
        path.join(homeDir, '.local', 'share', 'Steam', 'steamapps', 'common'),
        path.join(homeDir, '.var', 'app', 'com.valvesoftware.Steam', 'data', 'Steam', 'steamapps', 'common')
    ];

    for (const steamPath of steamPaths) {
        if (fs.existsSync(steamPath)) {
            try {
                const dirs = fs.readdirSync(steamPath);
                dirs.filter(d => d.startsWith('Proton')).forEach(d => {
                    const protonPath = path.join(steamPath, d, 'proton');
                    if (fs.existsSync(protonPath)) {
                        layers.push({ name: d, cmd: protonPath });
                    }
                });
            } catch (e) {}
        }
    }

    select.innerHTML = '';
    layers.forEach(l => {
        const opt = document.createElement('option');
        opt.value = l.cmd;
        opt.textContent = l.name;
        select.appendChild(opt);
        if (l.cmd === savedValue) opt.selected = true;
    });

    updateCompatDisplay();
}

function updateCompatDisplay() {
    const select = document.getElementById('compat-select');
    const display = document.getElementById('current-compat-display');
    if (select && display && select.selectedIndex !== -1) {
        display.textContent = select.options[select.selectedIndex].text;
    }
}

async function getInstalledPath() {
    const homeDir = require('os').homedir();
    const execPath = await Store.get('legacy_exec_path', DEFAULT_EXEC);
    return path.join(homeDir, 'Downloads', 'LegacyClient', execPath);
}

async function checkIsInstalled(tag) {
    const fullPath = await getInstalledPath();
    const installedTag = await Store.get('installed_version_tag');
    return fs.existsSync(fullPath) && installedTag === tag;
}

async function updatePlayButtonText() {
    const btn = document.getElementById('btn-play-main');
    if (isProcessing) return;

    if (isGameRunning) {
        btn.textContent = "GAME RUNNING";
        btn.classList.add('running');
        return;
    } else {
        btn.classList.remove('running');
    }

    const release = releasesData[currentReleaseIndex];
    if (!release) {
        btn.textContent = "PLAY";
        return;
    }

    if (await checkIsInstalled(release.tag_name)) {
        btn.textContent = "PLAY";
    } else {
        const fullPath = await getInstalledPath();
        if (fs.existsSync(fullPath)) {
            btn.textContent = "UPDATE";
        } else {
            btn.textContent = "INSTALL";
        }
    }
}

function updateRPC(details, state, startTime = null) {
    ipcRenderer.send('update-rpc', { details, state, startTime });
}

function setGameRunning(running) {
    isGameRunning = running;
    updatePlayButtonText();
    
    if (!running) {
        updateRPC('In Menus', 'Ready to Play');
    }
}

async function monitorProcess(proc) {
    if (!proc) return;
    const sessionStart = Date.now();
    setGameRunning(true);

    const release = releasesData[currentReleaseIndex];
    const version = release ? release.tag_name : 'Unknown';
    const isServer = await Store.get('legacy_is_server', false);
    updateRPC(`Playing Legacy (${version})`, isServer ? 'Running Headless Server' : 'In Game', sessionStart);

    proc.on('exit', async () => {
        const sessionDuration = Math.floor((Date.now() - sessionStart) / 1000);
        const playtime = await Store.get('legacy_playtime', 0);
        await Store.set('legacy_playtime', playtime + sessionDuration);
        setGameRunning(false);
    });
    proc.on('error', (err) => {
        console.error("Process error:", err);
        setGameRunning(false);
    });
}

function minimizeWindow() {
    ipcRenderer.send('window-minimize');
}

function toggleMaximize() {
    ipcRenderer.send('window-maximize');
}

function closeWindow() {
    ipcRenderer.send('window-close');
}

async function fetchGitHubData() {
    const repo = await Store.get('legacy_repo', DEFAULT_REPO);
    const loader = document.getElementById('loader');
    const loaderText = document.getElementById('loader-text');
    loader.style.display = 'flex';
    loaderText.textContent = "SYNCING: " + repo;

    try {
        const response = await fetch(`https://api.github.com/repos/${repo}/releases`);
        if (!response.ok) throw new Error("Rate Limited");

        releasesData = await response.json();
        populateVersions();

        setTimeout(() => {
            loader.style.opacity = '0';
            setTimeout(() => loader.style.display = 'none', 300);
        }, 500);
    } catch (err) {
        loaderText.textContent = "REPO NOT FOUND OR API ERROR";
        showToast("Check repository name in Options.");
        setTimeout(() => {
            loader.style.opacity = '0';
            setTimeout(() => loader.style.display = 'none', 300);
        }, 2500);
    }
}

function populateVersions() {
    const select = document.getElementById('version-select');
    const display = document.getElementById('current-version-display');
    select.innerHTML = '';

    if(releasesData.length === 0) {
        display.textContent = "No Releases Found";
        return;
    }

    releasesData.forEach((rel, index) => {
        const opt = document.createElement('option');
        opt.value = index;
        opt.textContent = `Legacy (${rel.tag_name})`;
        select.appendChild(opt);
        if(index === 0) display.textContent = opt.textContent;
    });
    currentReleaseIndex = 0;
    updatePlayButtonText();
}

function updateSelectedRelease() {
    const select = document.getElementById('version-select');
    currentReleaseIndex = select.value;
    document.getElementById('current-version-display').textContent = select.options[select.selectedIndex].text;
    updatePlayButtonText();
}

async function launchGame() {
    if (isProcessing || isGameRunning) return;

    const release = releasesData[currentReleaseIndex];
    if (!release) return;

    const asset = release.assets.find(a => a.name === TARGET_FILE);
    if (!asset) {
        showToast("ZIP Asset missing in this version!");
        return;
    }

    const isInstalled = await checkIsInstalled(release.tag_name);
    if (isInstalled) {
        setProcessingState(true);
        updateProgress(100, "Launching...");
        await launchLocalClient();
        setProcessingState(false);
    } else {
        const fullPath = await getInstalledPath();
        if (fs.existsSync(fullPath)) {
            const choice = await promptUpdate(release.tag_name);
            if (choice === 'update') {
                setProcessingState(true);
                await handleElectronFlow(asset.browser_download_url);
                setProcessingState(false);
            } else {
                setProcessingState(true);
                updateProgress(100, "Launching Existing...");
                await launchLocalClient();
                setProcessingState(false);
            }
        } else {
            setProcessingState(true);
            await handleElectronFlow(asset.browser_download_url);
            setProcessingState(false);
        }
    }

    updatePlayButtonText();
}

async function promptUpdate(newTag) {
    return new Promise(async (resolve) => {
        const modal = document.getElementById('update-modal');
        const confirmBtn = document.getElementById('btn-confirm-update');
        const skipBtn = document.getElementById('btn-skip-update');
        const installedTag = await Store.get('installed_version_tag', "Unknown");

        document.getElementById('update-modal-text').innerHTML = 
            `New version <b>${newTag}</b> is available.<br><br>` +
            `Currently installed: <b>${installedTag}</b>.<br><br>` +
            `Would you like to update now?`;

        modal.style.display = 'flex';
        modal.style.opacity = '1';

        const cleanup = (result) => {
            modal.style.opacity = '0';
            setTimeout(() => modal.style.display = 'none', 300);
            confirmBtn.onclick = null;
            skipBtn.onclick = null;
            resolve(result);
        };

        confirmBtn.onclick = () => cleanup('update');
        skipBtn.onclick = () => cleanup('launch');
    });
}

async function launchLocalClient() {
    const fullPath = await getInstalledPath();
    
    if (!fs.existsSync(fullPath)) {
        throw new Error("Executable not found! Try reinstalling.");
    }

    // Ensure the file is executable on Linux/macOS
    if (process.platform !== 'win32') {
        try {
            fs.chmodSync(fullPath, 0o755);
        } catch (e) {
            console.warn("Failed to set executable permissions:", e);
        }
    }

    return new Promise(async (resolve, reject) => {
        const compat = await Store.get('legacy_compat_layer', 'direct');
        const username = await Store.get('legacy_username', "");
        const ip = await Store.get('legacy_ip', "");
        const port = await Store.get('legacy_port', "");
        const isServer = await Store.get('legacy_is_server', false);

        let args = [];
        if (username) args.push("-name", username);
        if (isServer) args.push("-server");
        if (ip) args.push("-ip", ip);
        if (port) args.push("-port", port);

        const argString = args.map(a => `"${a}"`).join(" ");
        let cmd = `"${fullPath}" ${argString}`;
        
        if (process.platform === 'linux') {
            if (compat === 'wine64' || compat === 'wine') {
                cmd = `${compat} "${fullPath}" ${argString}`;
            } else if (compat.includes('Proton')) {
                const prefix = path.join(path.dirname(fullPath), 'pfx');
                if (!fs.existsSync(prefix)) fs.mkdirSync(prefix, { recursive: true });
                
                cmd = `STEAM_COMPAT_CLIENT_INSTALL_PATH="" STEAM_COMPAT_DATA_PATH="${prefix}" "${compat}" run "${fullPath}" ${argString}`;
            }
        }

        console.log("Launching command:", cmd);
        const startTime = Date.now();
        const proc = childProcess.exec(cmd, (error) => {
            const duration = Date.now() - startTime;
            if (error && duration < 2000) {
                showToast("Failed to launch: " + error.message);
                reject(error);
            } else {
                resolve();
            }
        });
        
        monitorProcess(proc);
    });
}

function setProcessingState(active) {
    isProcessing = active;
    const playBtn = document.getElementById('btn-play-main');
    const optionsBtn = document.getElementById('btn-options');
    const progressContainer = document.getElementById('progress-container');

    if (active) {
        playBtn.classList.add('disabled');
        optionsBtn.classList.add('disabled');
        progressContainer.style.display = 'flex';
        updateProgress(0, "Preparing...");
    } else {
        playBtn.classList.remove('disabled');
        optionsBtn.classList.remove('disabled');
        progressContainer.style.display = 'none';
    }
}

function updateProgress(percent, text) {
    document.getElementById('progress-bar-fill').style.width = percent + "%";
    if (text) document.getElementById('progress-text').textContent = text;
}

async function handleElectronFlow(url) {
    try {
        const homeDir = require('os').homedir();
        const downloadDir = path.join(homeDir, 'Downloads');
        const zipPath = path.join(downloadDir, TARGET_FILE);
        const extractDir = path.join(downloadDir, 'LegacyClient');

        updateProgress(5, "Downloading " + TARGET_FILE + "...");
        await downloadFile(url, zipPath);

        updateProgress(75, "Extracting Archive...");
        if (!fs.existsSync(extractDir)) {
            fs.mkdirSync(extractDir, { recursive: true });
        }
        await extractZip(zipPath, { dir: extractDir });

        const execName = await Store.get('legacy_exec_path', DEFAULT_EXEC);
        const fullPath = path.join(extractDir, execName);

        if (!fs.existsSync(fullPath)) {
            showToast("Executable not found at: " + execName);
            return;
        }

        updateProgress(100, "Launching...");
        
        await Store.set('installed_version_tag', releasesData[currentReleaseIndex].tag_name);
        
        await new Promise(r => setTimeout(r, 800));
        await launchLocalClient();

    } catch (e) {
        showToast("Error: " + e.message);
    }
}

function downloadFile(url, destPath) {
    return new Promise((resolve, reject) => {
        const dir = path.dirname(destPath);
        if (!fs.existsSync(dir)) {
            fs.mkdirSync(dir, { recursive: true });
        }

        const file = fs.createWriteStream(destPath);
        let totalSize = 0;
        let downloadedSize = 0;

        https.get(url, (response) => {
            if (response.statusCode === 302 || response.statusCode === 301) {
                downloadFile(response.headers.location, destPath).then(resolve).catch(reject);
                return;
            }

            totalSize = parseInt(response.headers['content-length'], 10);

            response.on('data', (chunk) => {
                downloadedSize += chunk.length;
                const percent = Math.floor((downloadedSize / totalSize) * 70) + 5;
                updateProgress(percent, `Downloading... ${percent}%`);
            });

            response.pipe(file);

            file.on('finish', () => {
                file.close(() => resolve());
            });

            file.on('error', (err) => {
                fs.unlink(destPath, () => {});
                reject(err);
            });
        }).on('error', (err) => {
            fs.unlink(destPath, () => {});
            reject(err);
        });
    });
}

function toggleOptions(show) {
    if (isProcessing) return;
    const modal = document.getElementById('options-modal');
    if (show) {
        modal.style.display = 'flex';
        modal.style.opacity = '1';
    } else {
        modal.style.opacity = '0';
        setTimeout(() => modal.style.display = 'none', 300);
    }
}

async function toggleProfile(show) {
    if (isProcessing) return;
    const modal = document.getElementById('profile-modal');
    if (show) {
        await updatePlaytimeDisplay();
        modal.style.display = 'flex';
        modal.style.opacity = '1';
    } else {
        modal.style.opacity = '0';
        setTimeout(() => modal.style.display = 'none', 300);
    }
}

async function updatePlaytimeDisplay() {
    const el = document.getElementById('playtime-display');
    const playtime = await Store.get('legacy_playtime', 0);
    if (el) el.textContent = formatPlaytime(playtime);
}

function formatPlaytime(seconds) {
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = seconds % 60;
    return `${h}h ${m}m ${s}s`;
}

async function saveOptions() {
    const newRepo = document.getElementById('repo-input').value.trim();
    const newExec = document.getElementById('exec-input').value.trim();
    const compatSelect = document.getElementById('compat-select');
    const ip = document.getElementById('ip-input').value.trim();
    const port = document.getElementById('port-input').value.trim();
    const isServer = document.getElementById('server-checkbox').checked;

    if (newRepo) await Store.set('legacy_repo', newRepo);
    if (newExec) await Store.set('legacy_exec_path', newExec);
    await Store.set('legacy_ip', ip);
    await Store.set('legacy_port', port);
    await Store.set('legacy_is_server', isServer);
    
    if (compatSelect) {
        await Store.set('legacy_compat_layer', compatSelect.value);
    }

    toggleOptions(false);
    fetchGitHubData();
    updatePlayButtonText();
    showToast("Settings Saved");
}

async function saveProfile() {
    const username = document.getElementById('username-input').value.trim();
    await Store.set('legacy_username', username);
    toggleProfile(false);
    showToast("Profile Updated");
}

function showToast(msg) {
    const t = document.getElementById('toast');
    t.textContent = msg;
    t.style.display = 'block';
    t.style.animation = 'none';
    t.offsetHeight; 
    t.style.animation = 'slideUp 0.3s ease-out';
    setTimeout(() => { 
        t.style.display = 'none';
    }, 3000);
}

// Global functions for HTML onclick
window.minimizeWindow = minimizeWindow;
window.toggleMaximize = toggleMaximize;
window.closeWindow = closeWindow;
window.launchGame = launchGame;
window.updateSelectedRelease = updateSelectedRelease;
window.toggleProfile = toggleProfile;
window.toggleOptions = toggleOptions;
window.saveOptions = saveOptions;
window.saveProfile = saveProfile;
window.updateCompatDisplay = updateCompatDisplay;
