
-----

# 💡 Animated LED Matrix


This project implements a custom controller for a **16x16 addressable LED matrix** using an **ESP32** to display high-framerate animations.
## Demo

https://v.redd.it/2gezibfc8v6g1/CMAF_1080.mp4

## Inspiration

This work was inspired by [Pipplee](https://pipplee.com/) - a project that displays animated GIFs on WLED installations.

## Rationale for a Separate Project

While Pipplee is excellent for managing an animation catalog, its reliance on WLED's preset system introduces two major limitations that this custom solution addresses:

### 1\. Upload/Load Speed

Pipplee loads animations by repeatedly uploading and editing WLED presets. According to the [WLED documentation](https://kno.wled.ge/features/presets/), frequent, random writes to the LittleFS preset file are **highly resource-intensive**.

> *...random writes to the preset file are very resource intensive. This means that while updating your presets, you might notice your light freezing and becoming unresponsive for up to a few seconds.*

Because Pipplee uploads a new preset every time an animation is switched, the system frequently becomes unresponsive, leading to a frustrating user experience.

### 2\. Framerate Limitation

Pipplee generates a separate WLED preset for every frame of an animation and uses a playlist to cycle through them.

The playlist feature in WLED imposes a **minimal preset duration of 0.2 seconds**, locking the maximum framerate at **5 FPS**. This project bypasses the WLED playlist limitation to achieve a significantly smoother animation display.

## Controls

The system is controlled using two rotary encoders, each with an integrated push button:

| Encoder | Rotation | Press  | Long Press | Pressed + Rotation Action |
| :--- | :--- | :--- | :--- | :--- |
| **Encoder 1 (Left)** | Switch the current animation or clock | Toggle automatic animation switching on/off | Switch betweeen animation and clock modes | N/A |
| **Encoder 2 (Right)** | Change the auto-switch frequency (1–60 seconds) | Pause the animation at the current frame | Set brigtness to 0| Adjust overall brightness |

## Circuit

### Connection Summary Table

The circuit utilizes two rotary encoders and an external power supply for the LED matrix.

| Component | Signal/Pin Name | ESP32 GPIO Pin | Constant Name | Connection Details |
| :--- | :--- | :--- | :--- | :--- |
| **LED Matrix** | Data Input ($D\text{IN}$) | **GPIO 13** | `DATAPIN` | Connected via a **330 $\Omega$ Resistor** to the LED matrix data line. |
| **LED Matrix** | Power ($V\text{CC}$) | **VIN** | N/A | Connects to the **external 5V** power supply rail. |
| **LED Matrix** | Ground ($G\text{ND}$) | **GND** | N/A | Connects to the main system ground. |
| **Bulk Capacitor** | ($+$) | **VIN** | N/A | A **1000 $\mu\text{F}$ 10V Capacitor** placed across VIN and GND for power smoothing. |
| **Encoder 1 (Left)** | A Channel ($A$) | **GPIO 14** | `ENC1_A` | Data Input (internal pull-up recommended). |
| | B Channel ($B$) | **GPIO 27** | `ENC1_B` | Data Input (internal pull-up recommended). |
| | Button ($S\text{W}$) | **GPIO 26** | `ENC1_BUTTON` | Data Input (internal pull-up recommended). |
| **Encoder 2 (Right)** | A Channel ($A$) | **GPIO 33** | `ENC2_A` | Data Input (internal pull-up recommended). |
| | B Channel ($B$) | **GPIO 25** | `ENC2_B` | Data Input (internal pull-up recommended). |
| | Button ($S\text{W}$) | **GPIO 32** | `ENC2_BUTTON` | Data Input (internal pull-up recommended). |

> 📌 **Note:** I used encoders which do not need power pins - only **GND** and **INPUT_PULLUP** type inputs.


### Wifi control

Using [tzapu/WiFiManager](https://github.com/tzapu/WiFiManager)
Initially WiFi access point **NEOMATRIX_AP** will be available, where you can connect and configure your wifi connection. After this AP is unavailable.

In the event Neomatrix cannot connect to Wifi in 30 seconds after boot - configuration AP will be available agein.

IP address can be discovered from log on serial connection or from your router's DHCP status page. 
On the web page you can select animation, contro animation auto advance and switch between animations/clock modes.

### Circuit Image

The full interactive circuit diagram is available at: [cirkitdesigner url](https://app.cirkitdesigner.com/project/41ab95f8-726e-4743-9e6f-fff42ea1a658)

![circuit](img/circuit.png "Circuit")

### Other circuit considerations
- You can use external power supply connected to LED matrix power inputs
- Make sure to use buck converter between matrix + terminal and ESP32 VIN if your LED matrix is 12V!
- When using WS2812b matrices, signal level converter such as SN74 is often recommended, but I don't have one and adding 330 Ohm resistor seems to work fine.

## 3D printed frame
I remixed [this](https://makerworld.com/en/models/122272-16x16-led-matrix-frame-with-diffuser-grid#profileId-131812) model to create [frame with encoders](https://makerworld.com/en/models/2109245-16x16-led-matrix-frame-with-encoders)
## Example build

![build](img/frame.jpeg "Frame")

## Animations Converter

The project includes a Python-based animation converter that transforms WLED presets (generated by Pipplee) into a C header file containing a **PROGMEM** array. This stores the animation data directly in the ESP32's flash memory for fast access.

 Animations I like are included in the repository. If you want more animations - respect the original developer who created hundreds of animations and an app to create animations from gif and purchase $3/year Pipplee license.

The conversion process is as follows:

1.  Load the desired animation into WLED.
2.  Save the preset (**Config -\> Security & Updates -\> Backup & Restore -\> Backup presets**).
3.  Rename the resulting JSON file and place it in the project's `animations_src` directory.
4.  Animations will be converted to header files in `include/animations` folder (which is cleaned by conversion script) on every build

If you want to try running conversion manually:
1.  Run the converter: `python3 scripts/convert.py`.


## Flash Requirements and Partitions

Animations data is stored usin RGB565 format which uses 16bits for every pixel.
A 16x16 matrix frame requires $16 \times 16 \times 2 \text{ bytes} = 512 \text{ bytes}$ of storage.

The standard ESP32 4MB flash partition table reserves approximately 1.5MB for the application (app0) and 1.5MB for a second application (app1).

Currently APP without animations uses **~1.16 MB** of flash. This leaves **~300 KB** for animations (**~600** frames or about 30 animations). Full length of every animation is in the end of this file.
Inluded 63 animation use **~960 KB** which translates to total **2.1 MB**

That's why **custom partition table** is used whith 3 MB app0 parition.



## BOM

| Component | Price, €$ | Link |
|---|---|---|
|ESP32 Devkit|4|[Aliexpress](https://www.aliexpress.com/item/1005008819591380.html)|
|16x16 LED Matrix|7|[Aliexpress](https://www.aliexpress.com/item/4000544584524.html?spm=a2g0o.order_list.order_list_main.5.3a121802zOs6bk)|
|2 Encoders |1.5|[Aliexpress](https://www.aliexpress.com/item/1005010063290644.html)|
|3D printed frame |3|[Makerworld](https://makerworld.com/en/models/2109245-16x16-led-matrix-frame-with-encoders#profileId-2281764)|
|1000 uF capacitor|0.1||
|330 Ohm resistor |0.05||
-----
## Animations Lengths
| File | Frames |
| :---  | :--- |
| eye_scan | 24 |
| cats_walking | 56 |
| monochrom_smiley | 60 |
| pirate_flag | 23 |
| candle | 5 |
| amongus | 6 |
| dinos_colors | 48 |
| penguin | 20 |
| loading | 17 |
| game_over | 57 |
| figures_tetris | 37 |
| lemur | 19 |
| cat | 45 |
| jumping_duck | 9 |
| ps_symbols | 16 |
| fireworks | 60 |
| chip | 16 |
| mtv | 4 |
| tetris | 60 |
| smiley | 30 |
| coffee | 22 |
| wow | 60 |
| rainbow_chekered | 14 |
| ducks_colors | 28 |
| dino | 25 |
| countdown | 60 |
| nemo | 59 |
| queen | 56 |
| eyes_pop | 11 |
| waves | 14 |
| sonic | 24 |
| golden_ring | 18 |
| laughing_minion | 53 |
| frog | 58 |
| red_heart | 5 |
| netflix | 32 |
| flash | 12 |
| rainbow_skull | 10 |
| snake_eye | 52 |
| gnome | 24 |
| santa_eating_candy | 39 |
| matrix | 8 |
| halloween | 16 |
| sponge_bob | 48 |
| pokeball | 30 |
| barbers | 12 |
| plane_window | 15 |
| hearts | 16 |
| beer | 42 |
| minion | 9 |
| parrot | 3 |
| shark | 31 |
| smiley_with_a_tongue | 5 |
| uss_enterprise | 48 |
| abduction | 44 |
| colorful_gates | 10 |
| stop | 16 |
| spiderman | 60 |
| licking_lips | 45 |
| jackson | 58 |
| panda_eating_grass | 33 |
| charlie_chaplin | 25 |
| christmas_tree | 16 |