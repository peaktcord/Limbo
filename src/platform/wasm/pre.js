Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(function () {
    const dependency = 'oblivion-idbfs-load';
    addRunDependency(dependency);
    try {
        try {
            FS.mkdir('/limbo');
        } catch (error) {
            if (!FS.analyzePath('/limbo').exists) throw error;
        }
        FS.mount(IDBFS, { autoPersist: true }, '/limbo');
        FS.syncfs(true, function (error) {
            if (error) {
                Module['oblivionLogError']('Could not restore persistent game data: ' + error);
                oblivionStorageFailed();
            } else {
                Module['oblivionPersistenceReady'] = true;
            }
            removeRunDependency(dependency);
        });
    } catch (error) {
        Module['oblivionLogError']('Persistent browser storage is unavailable: ' + error);
        oblivionStorageFailed();
        removeRunDependency(dependency);
    }
});

function oblivionStorageFailed() {
    Module['oblivionPersistenceReady'] = false;
    Module['oblivionStorageWarning'] = 'Browser storage failed. Progress will not be saved.';
    Module['oblivionSetStatus'](Module['oblivionStorageWarning'], false);
}

var oblivionFlushPending = false;
var oblivionFlushRunning = false;
var oblivionFlushAgain = false;

function oblivionRunFlush() {
    if (oblivionFlushRunning) {
        oblivionFlushAgain = true;
        return;
    }
    oblivionFlushRunning = true;
    FS.syncfs(false, function (error) {
        oblivionFlushRunning = false;
        if (error) {
            Module['oblivionLogError']('Could not save game data to browser storage: ' + error);
            oblivionStorageFailed();
        }
        if (oblivionFlushAgain && Module['oblivionPersistenceReady']) {
            oblivionFlushAgain = false;
            oblivionRunFlush();
        }
    });
}

Module['oblivionFlushPersistent'] = function () {
    if (!Module['oblivionPersistenceReady']) return;
    if (oblivionFlushPending) return;
    oblivionFlushPending = true;
    setTimeout(function () {
        oblivionFlushPending = false;
        oblivionRunFlush();
    }, 0);
};

if (typeof window !== 'undefined') {
    window.addEventListener('pagehide', function () {
        if (Module['oblivionPersistenceReady']) oblivionRunFlush();
    });
    document.addEventListener('visibilitychange', function () {
        if (document.visibilityState === 'hidden' && Module['oblivionPersistenceReady']) {
            oblivionRunFlush();
        }
    });
}

Module['oblivionImportJar'] = async function (file) {
    if (!file) return;
    const button = document.getElementById('choose-jar');
    const input = document.getElementById('jar-file');
    button.disabled = true;
    Module['oblivionSetStatus']('Reading and verifying ' + file.name + '…', true);

    try {
        try {
            FS.mkdir('/incoming');
        } catch (error) {
            if (!FS.analyzePath('/incoming').exists) throw error;
        }
        const bytes = new Uint8Array(await file.arrayBuffer());
        FS.writeFile('/incoming/oblivion.jar', bytes);

        await new Promise(function (resolve) { requestAnimationFrame(resolve); });
        const imported = Module['_oblivion_import_selected_jar']();
        if (imported && Module['oblivionPersistenceReady']) {
            await new Promise(function (resolve, reject) {
                FS.syncfs(false, function (error) {
                    if (error) reject(error); else resolve();
                });
            });
        }
    } catch (error) {
        Module['oblivionLogError']('Importing ' + file.name + ': ' + error);
        Module['oblivionSetStatus']('Import failed: ' + error, true);
    } finally {
        try {
            FS.unlink('/incoming/oblivion.jar');
        } catch (error) {
            if (FS.analyzePath('/incoming/oblivion.jar').exists) {
                Module['oblivionLogError']('Removing temporary JAR: ' + error);
            }
        }
        input.value = '';
        button.disabled = false;
    }
};
