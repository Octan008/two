// Shell
function Shell(a0, a1, a2) {
    ensureCache.prepare();
    if (a1 === undefined) { assert(typeof a0 === 'string', '[ERROR] Shell(0:resource_path): expected string'); }
    else if (a2 === undefined) { assert(typeof a0 === 'string', '[ERROR] Shell(0:resource_path): expected string'); assert(typeof a1 === 'string', '[ERROR] Shell(1:exec_path): expected string'); }
    else { assert(typeof a0 === 'string', '[ERROR] Shell(0:resource_path): expected string'); assert(typeof a1 === 'string', '[ERROR] Shell(1:exec_path): expected string'); assert(typeof a2 === 'boolean', '[ERROR] Shell(2:window): expected boolean'); }
    if (a1 === undefined) { this.__ptr = _mud_Shell__construct_1(ensureString(/*resource_path*/a0)); this.__type = Shell.__type; getCache(Shell)[this.__ptr] = this; }
    else if (a2 === undefined) { this.__ptr = _mud_Shell__construct_2(ensureString(/*resource_path*/a0), ensureString(/*exec_path*/a1)); this.__type = Shell.__type; getCache(Shell)[this.__ptr] = this; }
    else { this.__ptr = _mud_Shell__construct_3(ensureString(/*resource_path*/a0), ensureString(/*exec_path*/a1), /*window*/a2); this.__type = Shell.__type; getCache(Shell)[this.__ptr] = this; }
};
Shell.prototype = Object.create(WrapperObject.prototype);
Shell.prototype.constructor = Shell;
Shell.prototype.__class = Shell;
Shell.__cache = {};
Module['Shell'] = Shell;
Shell.prototype["init"] = Shell.prototype.init = function(a0) {
    if (a0 === undefined) {  }
    else { assert(typeof a0 === 'boolean', '[ERROR] init(0:window): expected boolean'); }
    if (a0 === undefined) { _mud_Shell_init_0(this.__ptr); }
    else { _mud_Shell_init_1(this.__ptr, /*window*/a0); }
};
Shell.prototype["window"] = Shell.prototype.window = function(a0, a1, a2) {
    ensureCache.prepare();
    if (a2 === undefined) { assert(typeof a0 === 'string', '[ERROR] window(0:name): expected string'); assert(a1.__type === v2_uint.__type, '[ERROR] window(1:size): expected v2<uint>'); }
    else { assert(typeof a0 === 'string', '[ERROR] window(0:name): expected string'); assert(a1.__type === v2_uint.__type, '[ERROR] window(1:size): expected v2<uint>'); assert(typeof a2 === 'boolean', '[ERROR] window(2:fullscreen): expected boolean'); }
    if (a2 === undefined) { return wrapPointer(_mud_Shell_window_2(this.__ptr, ensureString(/*name*/a0), /*size*/a1.__ptr), ShellWindow); }
    else { return wrapPointer(_mud_Shell_window_3(this.__ptr, ensureString(/*name*/a0), /*size*/a1.__ptr, /*fullscreen*/a2), ShellWindow); }
};
Shell.prototype["add_file"] = Shell.prototype.add_file = function(a0, a1) {
    ensureCache.prepare();
    assert(typeof a0 === 'string', '[ERROR] add_file(0:path): expected string'); assert(a1.__type === span_uint8_t.__type, '[ERROR] add_file(1:data): expected span<uint8_t>');
    _mud_Shell_add_file_2(this.__ptr, ensureString(/*path*/a0), ensureInt8(/*data*/a1), /*data*/a1.length);
};
Shell.prototype["begin_frame"] = Shell.prototype.begin_frame = function() {
    
    return !!(_mud_Shell_begin_frame_0(this.__ptr));
};
Shell.prototype["end_frame"] = Shell.prototype.end_frame = function() {
    
    _mud_Shell_end_frame_0(this.__ptr);
};
Shell.prototype["pump"] = Shell.prototype.pump = function() {
    
    return !!(_mud_Shell_pump_0(this.__ptr));
};
Shell.prototype["main_window"] = Shell.prototype.main_window = function() {
    
    return wrapPointer(_mud_Shell_main_window_0(this.__ptr), ShellWindow);
};
Object.defineProperty(Shell.prototype, "exec_path", {
    get: function() {
        return UTF8ToString(_mud_Shell__get_exec_path(this.__ptr));
    },
    set: function(value) {
        _mud_Shell__set_exec_path(this.__ptr, ensureString(value));
    }
});
Object.defineProperty(Shell.prototype, "resource_path", {
    get: function() {
        return UTF8ToString(_mud_Shell__get_resource_path(this.__ptr));
    },
    set: function(value) {
        _mud_Shell__set_resource_path(this.__ptr, ensureString(value));
    }
});
Object.defineProperty(Shell.prototype, "job_system", {
    get: function() {
        return wrapPointer(_mud_Shell__get_job_system(this.__ptr), JobSystem);
    }});
Object.defineProperty(Shell.prototype, "gfx", {
    get: function() {
        return wrapPointer(_mud_Shell__get_gfx(this.__ptr), GfxSystem);
    }});
Object.defineProperty(Shell.prototype, "editor", {
    get: function() {
        return wrapPointer(_mud_Shell__get_editor(this.__ptr), ShellContext);
    },
    set: function(value) {
        _mud_Shell__set_editor(this.__ptr, value.__ptr);
    }
});
Shell.prototype["__destroy"] = Shell.prototype.__destroy = function() {
    _mud_Shell__destroy(this.__ptr);
};
// ShellContext
function ShellContext() {
    
    this.__ptr = _mud_ShellContext__construct_0(); this.__type = ShellContext.__type; getCache(ShellContext)[this.__ptr] = this;
};
ShellContext.prototype = Object.create(WrapperObject.prototype);
ShellContext.prototype.constructor = ShellContext;
ShellContext.prototype.__class = ShellContext;
ShellContext.__cache = {};
Module['ShellContext'] = ShellContext;
Object.defineProperty(ShellContext.prototype, "screen", {
    get: function() {
        return wrapPointer(_mud_ShellContext__get_screen(this.__ptr), Widget);
    },
    set: function(value) {
        _mud_ShellContext__set_screen(this.__ptr, value.__ptr);
    }
});
Object.defineProperty(ShellContext.prototype, "dockbar", {
    get: function() {
        return wrapPointer(_mud_ShellContext__get_dockbar(this.__ptr), Dockbar);
    },
    set: function(value) {
        _mud_ShellContext__set_dockbar(this.__ptr, value.__ptr);
    }
});
ShellContext.prototype["__destroy"] = ShellContext.prototype.__destroy = function() {
    _mud_ShellContext__destroy(this.__ptr);
};
// ShellWindow
function ShellWindow() { throw "cannot construct a ShellWindow, no constructor in IDL" }
ShellWindow.prototype = Object.create(GfxWindow.prototype);
ShellWindow.prototype.constructor = ShellWindow;
ShellWindow.prototype.__class = ShellWindow;
ShellWindow.__cache = {};
Module['ShellWindow'] = ShellWindow;
Object.defineProperty(ShellWindow.prototype, "index", {
    get: function() {
        return _mud_ShellWindow__get_index(this.__ptr);
    },
    set: function(value) {
        _mud_ShellWindow__set_index(this.__ptr, value);
    }
});
Object.defineProperty(ShellWindow.prototype, "ui_window", {
    get: function() {
        return wrapPointer(_mud_ShellWindow__get_ui_window(this.__ptr), UiWindow);
    }});
Object.defineProperty(ShellWindow.prototype, "ui", {
    get: function() {
        return wrapPointer(_mud_ShellWindow__get_ui(this.__ptr), Ui);
    },
    set: function(value) {
        _mud_ShellWindow__set_ui(this.__ptr, value.__ptr);
    }
});
ShellWindow.prototype["__destroy"] = ShellWindow.prototype.__destroy = function() {
    _mud_ShellWindow__destroy(this.__ptr);
};

(function() {
    function setup() {
        Shell.__type = _mud_Shell__type();
        ShellContext.__type = _mud_ShellContext__type();
        ShellWindow.__type = _mud_ShellWindow__type();
    }
    if (Module['calledRun']) setup();
    else addOnPreMain(setup);
})();
