# HAI928I - Game Engine

Moteur de jeu 3D développé en C++ avec OpenGL, utilisant une architecture ECS (Entity Component System) et rendu raytracing.

## 🎮 Fonctionnalités

### Architecture ECS
- **EntityManager** : gestion centralisée des entités et composants
- **Composants** :
  - `TransformComponent` : position, rotation, scale, hiérarchie parent-enfant
  - `MeshComponent` : géométrie (primitives ou fichiers OFF), bounding sphere pour frustum culling
  - `MaterialComponent` : couleurs et textures
  - `CameraComponent` : caméra perspective avec contrôle FPS
  - `LightComponent` : éclairage directionnel
  - `AudioComponent` : audio spatial (musique, SFX, spatial)
  - `ControllerComponent` : contrôles utilisateur
  - `LuaScriptComponent` : scripts Lua attachés aux entités

### Systèmes
- **RenderSystem** : rendu avec frustum culling, shaders GLSL
- **TransformSystem** : mise à jour des matrices world avec hiérarchies
- **LightSystem** : gestion de l'éclairage
- **AudioSystem** : audio 3D avec OpenAL (listener suit la caméra active)
- **ScriptSystem** : exécution de scripts Lua (init, update, input)
- **ControllerSystem** : gestion des entrées clavier/souris (WASD, ESC)

### Rendu
- Shaders vertex/fragment programmables
- Frustum culling avec bounding spheres
- Support textures (avec fallback pour NPOT)
- Matériaux avec couleurs/textures

### Audio (En développement)
- 3 types : `MUSIC`, `SFX`, `SPATIAL`
- Audio spatial avec atténuation distance
- Support formats WAV
- Contrôle volume, loop, play on start

### Scripting Lua (En développement)
- API exposée : `Transform`, `Camera`, `Material`
- Callbacks : `onInit()`, `onUpdate(deltaTime)`, `onInput(event)`
- Exemple : cycle jour/nuit (SunCycle.lua)

### Chargement de scènes
- Format JSON (`scene.json`)
- Support meshes OFF et primitives (PLANE, SPHERE, BOX, CYLINDER, CONE, CAPSULE)
- Hiérarchies d'entités (parent-children)

### Mode Benchmark
- Test de performance avec grille de cubes
- Mesure FPS moyenne sur intervalles de 15s
- Augmentation progressive du nombre d'objets (×1.5)
- Arrêt automatique si FPS < 24

## 🛠️ Compilation

### Prérequis 
- **Compiler** : g++ avec support C++17
- **Bibliothèques (compilées automatiquement)** :
  - OpenGL 3.3+
  - GLEW
  - GLFW3
  - GLM
  - OpenAL
  - Lua 5.3+
  - nlohmann/json (header-only)

### Ubuntu/Debian
```bash
sudo apt-get install libglew-dev libglfw3-dev libglm-dev libopenal-dev liblua5.3-dev nlohmann-json3-dev
```

### Build
```bash
cd Engine
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

## 🚀 Utilisation

### Lancer le jeu (et build automatique)
```bash
./execute.sh ../Jeu/
```

### Mode benchmark
```bash
./execute.sh ../Jeu/ -b
```

### Contrôles
- **WASD** : déplacement caméra (si `Controller` attaché)
- **Souris** : orientation caméra
- **ESC** : quitter

## 📁 Structure du projet

```
HAI928I/
├── Engine/                     # Code source du moteur
│   ├── main.cpp               # Point d'entrée, boucle de rendu
│   ├── Components/            # Composants ECS
│   │   ├── TransformComponent.h
│   │   ├── MeshComponent.h
│   │   ├── MaterialComponent.h
│   │   ├── CameraComponent.h
│   │   ├── LightComponent.h
│   │   ├── AudioComponent.h
│   │   └── LuaScriptComponent.h
│   ├── Systems/               # Systèmes ECS
│   │   ├── EntityManager.h    # Gestionnaire ECS
│   │   ├── RenderSystem.h
│   │   ├── TransformSystem.h
│   │   ├── LightSystem.h
│   │   ├── AudioSystem.h
│   │   ├── ScriptSystem.h
│   │   └── ControllerSystem.h
│   ├── Entities/
│   │   └── Entity.h           # ID d'entité (uint32_t)
│   ├── Shaders/
│   │   ├── vertex_shader.glsl
│   │   └── fragment_shader.glsl
│   └── execute.sh             # Script de lancement
├── Jeu/                       # Assets du jeu
│   ├── scene.json             # Définition de la scène
│   ├── mesh/                  # Modèles 3D (.off)
│   ├── textures/              # Textures
│   ├── audios/                # Fichiers audio (.wav)
│   └── Scripts/               # Scripts Lua
│       └── SunCycle.lua       # Exemple de script
└── readme.md
```

## 📝 Format de scène (scene.json)

```json
{
  "entities": [
    {
      "id": 0,
      "transform": {
        "position": [0, 0, 0],
        "rotation": [0, 0, 0],
        "scale": [1, 1, 1],
        "parent": null,
        "children": []
      },
      "mesh": {
        "type": "primitive",
        "mesh_type": "SPHERE",
        "subdivisions": 100
      },
      "material": {
        "type": "color",
        "color": [1.0, 0.5, 0.0]
      },
      "light": {
        "intensity": 1.0
      },
      "camera": {
        "idCam": 0,
        "target": [0, 0, -1],
        "up": [0, 1, 0],
        "fov": 45.0,
        "near_plane": 0.1,
        "far_plane": 100.0
      },
      "audio": {
        "type": "music",
        "path": "audios/soundtrack.wav",
        "volume": 0.5,
        "loop": true,
        "play_on_start": true
      },
      "script": {
        "type": "Lua",
        "path": "Scripts/SunCycle.lua"
      },
      "controller": {
        "speed": 5.0
      }
    }
  ]
}
```

## ⚡ Optimisations

- **Frustum culling** : ne dessine que les objets visibles
- **Bounding spheres** : calcul automatique lors du chargement des meshes
- **Hiérarchie de transforms** : mise à jour incrémentale (dirty flag)
- **ECS** : itération cache-friendly sur les composants

## 🐛 Debug

### Rien ne s'affiche
1. Vérifier que la caméra a `isActive = true`
2. Vérifier position/rotation caméra (doit voir les objets)
3. Désactiver frustum culling temporairement
4. Vérifier que `vertexCount > 0` et `VAO != 0`

### Crash `std::out_of_range`
1. Vérifier que `EntityManager.CreateEntity(id)` est appelé avant `AddComponent`
2. Vérifier que les composants existent avant `GetComponent`
3. Utiliser `.find()` au lieu de `.at()` pour accès sûr

### Audio ne marche pas
1. Vérifier chemins des fichiers WAV
2. Vérifier que `AudioSystem` est initialisé après `loadScene()`
3. Vérifier que la caméra active a un `TransformComponent`

## 📊 Performances (Benchmark)

Exemple de sortie :
```
--- Benchmark scene with 50 cubes loaded. ---
Moyenne FPS pour 50 cubes : 120.3 et 48400 triangles.
--- Benchmark scene with 75 cubes loaded. ---
Moyenne FPS pour 75 cubes : 95.7 et 72600 triangles.
--- Benchmark scene with 112 cubes loaded. ---
Moyenne FPS pour 112 cubes : 68.4 et 108416 triangles.
--- Benchmark scene with 168 cubes loaded. ---
Moyenne FPS pour 168 cubes : 42.1 et 162624 triangles.
--- Benchmark scene with 252 cubes loaded. ---
Moyenne FPS pour 252 cubes : 22.8 et 243984 triangles.
--- Benchmark terminé. ---
```

## 📄 Licence

Projet universitaire - HAI928I M2 Informatique

## 👥 Contributeurs
Dupuis Thibaut - Langouet Bastian
Développé dans le cadre du Master 2 Informatique - Université de Montpellier