

#ifndef CLIENT_H
#define CLIENT_H

#include "engine_settings.h"
#include "platform.h"
#include "render_system.h"
#include "snd_system.h"
#include "nav_system.h"
#include "gui_system.h"
#include "part_system.h"
#include "scene_system.h"
#include "input_system.h"
#include "player.h"

#include "data_assets.h"

typedef struct
{
    const char* name;
    int value;
} EnumTable;

typedef struct
{
    const EnumTable* propEnumTable;
    const EnumTable* eventEnumTable;
} LevelLoadTable;

/*
 Engine conducts interactions between systems.
 Engine is composed of systems. Systems are not aware of each other.
 
 Renderer requires read access to all systems.
 Therefore the renderer is not a system

 Gameplay requires read/write access to all systems.
 Therefore the "Game" and "Game Objects" aren't systems.
 
 GameObjects require access to each other
 */

#define ENGINE_PLAYER_COUNT 2

#define ENGINE_PLAYER_LOCAL 0
#define ENGINE_PLAYER_AI 1

/* Game and engine are not distinct. The engine is built specifically for the game. */

typedef struct Engine
{
    RenderSystem renderSystem;
    SndSystem soundSystem;
    NavSystem navSystem;
    PartSystem partSystem;
    GuiSystem guiSystem;
    InputSystem inputSystem;
    SceneSystem sceneSystem;

    const WeaponInfo* weaponTable;
    const LevelLoadTable* levelLoadTable;
    
    FogView fogView;

    EngineState state;
    EngineResult result;
    Command command;
    
    Player* players[ENGINE_PLAYER_COUNT];
        
    int turn;
    int paused;

} Engine;


extern Engine g_engine;

extern int Engine_Init(Engine* engine,
                       Renderer* renderer,
                       SndDriver* sndDriver,
                       EngineSettings settings,
                       Player* localPlayer,
                       Player* aiPlayer);

extern void Engine_Shutdown(Engine* engine);

extern void Engine_End(Engine* engine);

extern void Engine_Pause(Engine* engine);
extern void Engine_Resume(Engine* engine);

extern void Engine_RunCommand(Engine* engine, const Command* command);
extern Player* Engine_LocalPlayer(Engine* engine);

extern EngineState Engine_Tick(Engine* engine, const InputState* newState);
extern void Engine_Render(Engine* engine);


/*
  "If you want to be a game programmer, or for that matter any sort of programmer at all, 
  here’s the secret to success in just two words:  Ship it.
  Finish the product and get it out the door, and you’ll be a hero." 
  - Michael Abrash

 DONE
 =================================
  * Pull GlShader from WorldC - 5/21/16
  * Fog of war shader - 5/21/16
  * Programmable pipeline (GL 2.1) - 5/22/16
  * GPU particle system - 5/22/16
  * Document Nav stuff - 5/19/16
  * GUI design prototype - 5/19/16
  * WorldHandler refactor - 5/18/16
  * Decide if turn logic could be an independent system - 5/17/16
        It can't it's an integral part of the Client system.
  * particle system - 5/13/16
  * textures - 5/12/16
  * basic hurting animation and effects - 6/1/16
  * Changed export tool to work with kits - 6/14/16
  * Create "kit" prototype - 6/15/16
  * Characters visible through walls - 6/16/16
  * Fog of war gameplay with per controller perspective - 6/17/15
  * bullets shouldn't go through walls or floors - 6/17/16
  * fix export script lightmap index - 6/18/16
  * chunks should have correct origins - 6/18/16
  * Revise shader uniform lookup's - 6/23/16
  * OpenGL 3.2, Core profile compatibility - 6/24/16
  * Pass camera to renderer - 6/25/16
  * Change Hint rendering to GL_LINES instead of GL_LINE_LOOP - 6/25/16
  * glCullFace back and glClearColor should be in setup - 6/25/16
  * GlProg_FindUniform should be replaced with convenience - 6/25/116
  * emitters should be part of culling process - 6/25/16
  * emitters should pass control nodes to shader - 6/25/16
  * refactor object shade into uber shader. Also merged world shader into uber shader - 6/29/16
  * OpenGL ES 2.0 Renderer - 5/30/16
  * GPU mode for skinning - 7/1/16
  * skinned and static mesh loading/updating - 7/1/16
  * Normalize input coordinates - 7/3/16
  * Indexed and ordererd drawing of GUI and labels - 7/4/16
  * Separate and index drawing of Hints - 7/6/16
  * change gui to use hierarchy - 7/8/16
  * touch input - 7/8/16
  * PVRTC Texture support - 7/9/16
  * Replace Nav Solver list - 7/10/16
  * Fix Part emitters on iOS - 7/11/16
  * transform normals for skeletons - 7/13/16
  * create uniform index table - 7/16/16
  * fix unit visibility - 7/17/16
        units were being sorted by the culling process, but corresponding visibility values were not
  * asset loading/unloading - 7/18/16
        I created the asset manifest tool that generates a manifest header for easy loading
  * weapon icon's into separate gui texture - 7/19/16
  * change chunk bounds to AABB - 7/20/16
  * change observers to light even when model is hidden - 7/20/16
  * fix prop orientation - 7/21/16
        Added attach system to skeletons
  * fix behind wall rendering - 7/21/16
        Behind wall rendering requires an additional pass for each unit, which must take place
        for every unit before actual unit renders can begin for layering purposes. 
        This causes additional context switches including a reupload of bone matricies for each pass.
        Combined with shadows, each unit requires 3 render passes, 2 of which should be    
        light on the fragment rendering. Behind wall rendering is critical for the game.
        If performance ends up being a problem, projected shadows must be replaced with
        simple shadow circles, accumlated in a VBO and rendered in a single pass for all units.
  * fix bullet rotation - 7/27/16
  * fix skeleton root bone offset - 7/30/16
  * attack with both primary and secondary weapons - 8/1/16
  * fix bullets/damage so they spawn at the weapon tip - 8/3/16
  * Added alpha blending to UI (changed vec3 colors to vec4) - 8/10/16
  * Add menu background to UI texture - 8/11/16
  * Fix blender level, lightmap bake, z fighting - 8/13/16
        Chunk groups are being duplicated as they are joined. This causes two sets of meshes at every position.
  * Fix PVR coloring on iOS - 8/14/16
  * Create new level prototype - 8/21/16
  * Fix nasty lightmap seems - 8/21/16
  * Fix prop rotations from level - 8/22/16
  * Fix controller events so player can click outside of nav mesh - 9/1/16
  * Add NavMesh rendering to debug - 9/4/16
  * Fix strange corner path finding bug - 9/4/16
  * make recenter button - 9/4/16
  * Disable UI buttons during commands - 9/4/16
  * Stop black parts of levels from getting lightmapped - 9/26/16
        Fragment shaders can check if they are backfacing. Simply render back fragments black, cut extra faces frmo mesh.
  * Skel Meshes can now include attach points. 9/30/16
  * Use 16 bit integer storage for UV coordinates. 10/14/16
        Floats can be packed as integers and converted to floats with glVertexAttrib, using a normalized option or simple conversion
  * Fix bat attack so both hands can be used - 11/4/16
  * spawn particle effect - 11/4/16
  * teleport particle effect - 11/4/16
  * fix priority queues in nav - 11/12/16
  * replace custom buffer with stb stretchy buffer - 11/12/16
  * Add revolver to item texture - 11/18/16
  * asset unloading - 11/20/16
  * Fix core audio cleanup bugs - 11/15/16
  * Fix bug where prop transform is incorrect in release mode. - 11/15/16 (Order of object, translate matrices matters in prop? wtf)
  * Buy/Action button - 12/1/16
  * Fix unit prop after buying weapon - 12/1/16
  * Fade In/Fade Out 12/8/16
  * Fix strange witch joint animation bug - 12/10/16
  * Speed up phantom animations to correct speed - 12/11/16
  * Data drive all character attach points in skeleton model - 12/11/16
  * Witch teleport animation - 12/11/16
  * Fix lightmap gaps in PVR compression - 12/12/16
  * Batched together SkelAnim frame allocations for optimization - 12/13/16
  * randomize idle animation start frames - 12/13/16
  * make player start turn occur after first tick - 12/13/16
  * make eye button cycle through players - 12/13/16
  * add deselect button - 12/14/16
  * added ability to cancel attack button selection - 12/15/16 
  * fix attach point rotation - 12/16/16
  * design currency render - 12/17/16
  * add currency label - 12/17/16
  * fix bullet direction when aiming at close points - 12/18/16
  * setup first basic level with items - 12/18/16
  * change shadow plane to match nav mesh - 12/20/16
  * don't allow unit move to begin if the move counter has expired - 12/22/16
  * lightmap script should export RGB (with no alpha) lightmaps - 12/22/16
  * lightmap script should merge neighboring vertices - 12/22/16
  * added directional modified to bullet/melee damage - 12/23/16
  * added ranged enemy (vampire) - 12/23/16
  * witch hurt animation bug - 12/24/16
  * Tapping an enemies upper body should target him instead of shooting behind him - 12/26/16
  * attack buttons should allow you to deselect - 12/26/16
  * unit selection should be preseved between turns - 12/26/16
  * fix scientist hand offset - 12/26/16
  * Axe doesn't work when you get up close - 12/26/16
  * bug where ending turn while enemy is dying causes them to come back alive - 12/26/16
  * Panning should not tap - 12/27/16
  * unit perception/visibility angle - 12/27/16
  * stop hp from exceeding bar limits - 12/28/16
  * allow level properties to configure price of items and weapons - 12/28/16
  * design & program other kinds of powerup kits (defense and range) - 12/28/16
  * renders for unit selection tiles - 12/31/16
  * change lightmaps from 1024x1024 to 512x512 for disk usage - 12/31/16
  * props need to render outlines as well as units - 1/3/16
  * optimize world fragment shader - 1/3/16
        Changed to a constant size for loop. Adjusted visibility function to not require highp float operations.
  * prevent "doubling up" on powerups - 1/4/17
  * bug where dead props started drawing randomly - 1/4/17
  * level circle button image - 1/5/17
  * level rewards - 1/5/17
        An array in the level info determines which new levels are unlocked.
        Rewarding units was cut in favor of allowing purchases of units. Currency from levels is used to purchase new units.
  * limit the number of units that can be captured so that observers is not exceeded. - 1/5/17
  * bug where sometimes units attempt to move on impossible paths - 1/5/17
  * move range has expired sound - 1/5/17
  * Binary mesh format (__STDC_IEC_559__) - 1/6/17
        The loading times of the text format were fine, but filesize for levels started to become an issue.
  * Revolver and cannon are too similar in roles - 1/7/17
        I changed cannon to deliver the most damage at long range, but to do minimal damage at close range. This is the opposite of the revolver.
  * cannon explosion sound - 1/7/17
  * some back facing triangles in the levels are not black - 1/8/17
        caused by cracks in the lab kit
  * powerups need to be raised up off the floor - 1/9/17
        change Z position to 2.0 above floor
  * allow each unit type to have hurt/death sounds - 1/18/17
  * touchEnabled triggers - 1/20/17
  * fix gap in wooden stair kit - 1/20/17
  * vampires don't shoot across gaps - 1/22/17
  * allow deselect button to cancel moves - 1/22/17
  * mg up close sometimes spreads out shots too much - 1/22/17
        changed spread algorithim
  * change visibility from fragment shader to vertex shader - 2/3/17
  * visibility vertex shader is causing hole in level around - 2/20/17
  * mind control units shouldn't drop skulls - 2/20/17
  * bullets sometimes go through walls - 2/20/17
  * prevent tapping on empty weapons - 2/20/17
  * weapons need larger bounding boxes so they don't disappear from camera culling - 2/20/17
  * turn alert sound - 5/8/17
  * instructional overlay - 5/9/17
  * revised path finding for effeciency and correctness - 6/12/17
  * use materials instead of texture/model enums - 6/14/17
  * skeleton transitions should blend root transform - 6/16/17
  * pad level buttons so they are easily clickable - 6/19/17
  * make pistol strong at mid range (no distance drop off) - 6/19/17
  * low damage hit sound - 6/20/17
  * egg healing sound - 6/20/17
  * simplified blender tools - 6/20/17
  * fixed dark tool bug - 6/20/17
    * custom machete animation - 6/23/17
  * make units nameable - 6/24/17
  * track unit's successful misisons - 6/24/17
  * refactored damage system - 6/26/17
  * add random chanace to syringe - 6/26/17
  * currency design and overworld design needs work - 6/26/17
    Currency has been dropped and now units just need to capture control points to win.
    At the beginning of the game the player will receive a large number of units,
    These will act as "lives" which the player will spend on the entire game.
    If the player loses all his units, they can reset back to default (without losing their level progress).
  * user should be able to change path movement in the middle of a path - 6/27/17
  * spamming movement should not spam sounds - 6/27/17
  * "bad tap"/error sound - 6/27/17
  * egg healing AI movement - 7/1/17
  * program machete, area of effect damage - 7/1/17
  * added new eye mine - 7/1/17
  * new dynamic lighting system - 7/4/17
  * give/up lose overworld progress button - 7/7/17
  * footstep sounds - 7/8/17
  * bug where rotating to target causes extra bullets to fire - 7/8/17
  * emitters should use transform matrix instead of origin - 7/8/17
  * ogg vorbis support - 7/15/17
  * create a particle effect for vampire balls - 7/16/17
  * limit the types of enemies which the boss can spawn - 7/31/17
  * boss should keep his distance - 7/31/17
  * units sometimes rotate in directions that don't make sense - 8/5/17
  * navigation across two neighbor polygons at a steep angle can cross the nav mesh - 9/6/17
  * pistol isn't matched well to hands - 12/14/17
  * loading indicator and threading - 12/14/17
  * storage 2 spawns tons of bats - 12/15/17
        not an engine, bug it was actually in the file
  * instruction overlay is broken - 12/15/17
        It was a gray scale + alpha format that I didn't support
  * hide end turn when using weapon - 12/17/17
  * add sound for eye mine activation, and eye mine explode - 1/1/18
 
- https://developer.apple.com/library/mac/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/UpdatinganApplicationtoSupportOpenGL3/UpdatinganApplicationtoSupportOpenGL3.html#//apple_ref/doc/uid/TP40001987-CH3-SW1

 IDEAS
 =================================
  - FXAA fullscreen shader
  - Metal renderer
  - pan inertia
  - don't transform skeleton when it is not visible
  - teleporters
  - world specular map
 
  Biggest Time Sucks:
  1. Realizing code is poorly structured and compensating.
  2. Art asset creation/debugging.
 
 Assets
 =================
 
 Renders
 ----------------------
 * Phantom Claw Render
 * Mg Render
 * Axe Render
 * Cannon Render
 * Wolf Render
 * Bat Arm Render 
 * Magic Ball Render
 * Syringe Render
 * Revolver Render
 * Speed Pack Render
 * Attack Pack Render
 * Hp Kit Render
 * Icon Render
 * Overworld Render
 * Menu Render/Photoshop
 * Revolver render
 * Skull Render
 * Vampire ball render
 * scientist mg tile
 * scientist cannon tile
 * scientist revolver tile
 * scientist syringe tile
 * bat tile
 * vamp tile
 * phantom tile
 * wolf tile

 Characters
 ----------------------
 * scientist model
 * scientist rig
 * scientist texture
 * scientist idle
 * scientist walk
 * scientist hurt
 * scientist die
 * scientist axe attack
 * scientist mg shoot
 * scientist cannon shoot
 * scientist syringe shoot
 * scientist pistol idol
 * scientist pistol walk
 * scientist pistol shoot

 * phantom model
 * phantom rig
 * phantom texture
 * phantom idle
 * phantom walk
 * phantom attack
 * phantom hurt
 * phantom die
 
 * bat model
 * bat rig
 * bat texture
 * bat idle
 * bat walk
 * bat attack
 * bat hurt
 * bat die
 
 * wolf model
 * wolf rig
 * wolf texture
 * wolf idle
 * wolf walk
 * wolf attack
 * wolf hurt
 * wolf die
 
 * witch model
 * witch rig
 * witch texture
 * witch idle
 * witch walk 
 * witch attack
 * witch hurt
 * witch die
 * witch teleport
 
 Props
 --------------------
 * Mg Model
 * Mg Texture
 * Mg Bullet Model
 * Mg Bullet Texture
 * Mg Icon Render
 
 * Cannon Model
 * Cannon Texture
 * Cannon Bullet Model
 * Cannon Bullet Texture
 * Cannon Icon Render
 
 * Axe Model
 * Axe Texture
 * Axe Icon Render
 
 * Revolver Model
 * Revolver Texture
 
 * Health Pack Model
 * Health Pack Texture
 
 * Speed Pack Model
 * Speed Pack Texture
 
 * Attack Pack Model
 * Attack Pack Texture
 
 * Magic Ball Model
 * Magic Ball Texture
 
 * Skull Model
 * Skull Texture
 * Skull Icon Render
 
 Sounds
 --------------------
 * Scientist Effects
 * Bat Effects
 * Wolf Effects
 * Phantom Effects
 - Boss Effects
 
 - cannon sounds (improve)
 - vampire throw sounds (temporary)
 - shaman teleport sound (temporary)
 - eye mine sounds (temporary)
 - create another phantom select sound

 - storage ambient
 - lab ambient
 - plant ambient
 - observatory ambient
 
*/

#endif
