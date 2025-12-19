package com.adrcotfas.skiaseekbar

import org.libsdl.app.SDLActivity

class MainActivity : SDLActivity() {
    
    override fun getLibraries(): Array<String> {
        return arrayOf(
            "SDL3",
            "main"
        )
    }
}
