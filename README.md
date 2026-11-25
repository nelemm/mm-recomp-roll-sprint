# Roll Sprint

A [Zelda 64: Recompiled](https://github.com/Zelda64Recomp/Zelda64Recomp) mod for *Majora's Mask*: roll, keep **A** held, and Link keeps running at roll speed. Also adds modern, responsive turning.

[English](#english) · [Français](#français)

---

## English

### Features

- **Roll sprint**: roll (press **A** while running), keep **A** held, and Link runs at roll speed (×1.5, configurable). Release **A** or stop moving to go back to normal speed.
- **Consistent movement while sprinting**: turn rate, braking and acceleration are scaled with the sprint speed, so Link doesn't get stuck braking when you circle the stick.
- **Matching animation**: the run animation is sped up with the actual speed, so Link's feet don't slide.
- **Fast turning**: Link pivots almost on the spot (about 0.15 s for a 180° turn) instead of slowing down to a stop before turning around.

Only the regular running action is affected: Z-targeting, swimming, climbing, Goron and Deku movement are untouched.

### Settings

Available in the in-game **Mods** menu:

| Setting | Values | Default |
|---|---|---|
| Sprint apres roulade (roll sprint) | Active / Desactive | Active |
| Vitesse du sprint (sprint speed) | 1.0 – 2.5 (1.5 = roll speed) | 1.5 |
| Virages (turning) | Classiques / Rapides (sprint) / Rapides (toujours) / Instantanes | Rapides (toujours) |

Disabling the sprint also disables fast turning.

### Compatibility

- Uses hooks only (no `RECOMP_PATCH`), so it can be combined with other mods that change rolling or running, such as *Roll Master* or *Enhanced Roll*.
- Works with the Archipelago randomizer ([MMRecompRando](https://github.com/RecompRando/MMRecompRando)). It does not touch game logic, items, saves or network code.
- Requires Zelda 64: Recompiled 1.2.2 or newer.

### Installation

1. Download `nelem_mm_roll_sprint.nrm` from the [Releases](../../releases) page.
2. Copy it into your mods folder (`%LOCALAPPDATA%\Zelda64Recompiled\mods` on Windows).
3. Enable **Roll Sprint** in the game's **Mods** menu.

### Building

Requirements: `make`, a MIPS-capable `clang` and `ld.lld`, and `RecompModTool` from [N64Recomp](https://github.com/N64Recomp/N64Recomp).

```bash
git clone --recurse-submodules <this repository>
cd mm-recomp-roll-sprint
make
RecompModTool mod.toml build
```

The mod is written to `build/nelem_mm_roll_sprint.nrm`.

### How it works

| Hook | Purpose |
|---|---|
| `Player_Update` | Tracks the sprint state: roll with **A** held, then sprint until **A** is released or the stick stays neutral. |
| `Player_CalcSpeedAndYawFromControlStick` | Multiplies the target speed during the normal run action (`Player_Action_13`), like the roll does. |
| `Player_Action_13` | Temporarily scales the run registers (acceleration, turn rate, braking, animation speed) while sprinting, then restores them. |
| `Player_GetMovementSpeedAndYaw` | Fast turning: moves the movement direction towards the stick before the game decides to brake. |
| `Player_UpdateShapeYaw` | Keeps Link's body facing his movement direction at the same pace. |

### License

[CC0 1.0](LICENSE) — public domain. Based on the [MMRecompModTemplate](https://github.com/Zelda64Recomp/MMRecompModTemplate) (CC0) and the [zeldaret/mm](https://github.com/zeldaret/mm) decompilation.

---

## Français

### Fonctionnalités

- **Sprint après roulade** : fais une roulade (**A** en courant), garde **A** enfoncé, et Link continue à la vitesse de la roulade (×1,5, réglable). Relâche **A** ou arrête-toi pour revenir à la vitesse normale.
- **Déplacement cohérent en sprint** : rotation, freinage et accélération suivent la vitesse du sprint, donc Link ne reste plus bloqué à freiner quand tu fais des cercles avec le stick.
- **Animation synchronisée** : l'animation de course accélère avec la vitesse réelle, les pieds ne glissent plus.
- **Virages rapides** : Link pivote presque sur place (environ 0,15 s pour un demi-tour) au lieu de freiner jusqu'à l'arrêt.

Seule la course normale est concernée : visée Z, nage, escalade, formes Goron et Mojo ne changent pas.

### Réglages

Dans le menu **Mods** du jeu :

| Réglage | Valeurs | Par défaut |
|---|---|---|
| Sprint apres roulade | Active / Desactive | Active |
| Vitesse du sprint | 1,0 – 2,5 (1,5 = vitesse de la roulade) | 1,5 |
| Virages | Classiques / Rapides (sprint) / Rapides (toujours) / Instantanes | Rapides (toujours) |

Désactiver le sprint désactive aussi les virages rapides.

### Compatibilité

- Uniquement des hooks (aucun `RECOMP_PATCH`) : compatible avec d'autres mods qui modifient la roulade ou la course, comme *Roll Master* ou *Enhanced Roll*.
- Compatible avec le randomizer Archipelago ([MMRecompRando](https://github.com/RecompRando/MMRecompRando)). Le mod ne touche ni à la logique, ni aux objets, ni aux sauvegardes, ni au réseau.
- Nécessite Zelda 64: Recompiled 1.2.2 ou plus récent.

### Installation

1. Télécharge `nelem_mm_roll_sprint.nrm` depuis la page [Releases](../../releases).
2. Copie-le dans le dossier des mods (`%LOCALAPPDATA%\Zelda64Recompiled\mods` sous Windows).
3. Active **Roll Sprint** dans le menu **Mods** du jeu.

### Compilation

Voir la section [Building](#building) ci-dessus.

### Licence

[CC0 1.0](LICENSE) — domaine public.
