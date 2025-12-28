# Apes Banana Collection Simulation

A multi-threaded simulation of ape families collecting bananas in a maze using OpenGL/GLUT for visualization.

## Features

- **Multi-threaded simulation** using pthreads
- **OpenGL/GLUT graphics** for real-time visualization
- **Configurable parameters** via `config.txt`
- **Dynamic maze generation** with randomized obstacles and bananas
- **Complex interactions**: fights, stealing, energy management

## Requirements

- macOS with native GLUT framework (or Linux with freeglut)
- GCC compiler
- OpenGL libraries

## Building

```bash
make clean
make
```

## Running

```bash
./ape_simulation [config_file]
```

Default configuration file is `config.txt` if not specified.

## Controls

- **ESC** or **q**: Quit the simulation
- Graphics window automatically updates at ~30 FPS

## Graphics Display

The graphics window shows:
- **White cells**: Empty maze paths
- **Black cells**: Obstacles
- **Yellow cells**: Bananas
- **Pink rectangles**: Female apes (with banana count)
- **Blue rectangles**: Male apes (with energy level)
- **Brown rectangles**: Baby apes (with bananas eaten)

## Simulation Rules

### Female Apes
- Enter the maze to collect bananas
- Collect until reaching threshold or exhausting nearby bananas
- Return to basket protected by male ape
- Rest when energy drops below threshold
- May fight other females when leaving maze

### Male Apes
- Protect family basket
- Fight neighboring males (probability increases with basket size)
- Winner steals loser's bananas
- Family withdraws if male energy drops too low

### Baby Apes
- Steal bananas during male fights
- Either give to dad's basket or eat themselves
- Opportunistic behavior during chaos

## Termination Conditions

Simulation ends when any of these occurs:
- Too many families withdraw (default: 3)
- A family collects too many bananas (default: 50)
- A baby eats too many bananas (default: 15)
- Simulation runs too long (default: 60 seconds)

## Configuration File

Edit `config.txt` to customize:
- Maze dimensions
- Number of families
- Energy thresholds
- Banana distribution
- Termination conditions
- And more...

## Notes

- OpenGL is deprecated on macOS 10.14+ but still functional
- Graphics use native macOS GLUT framework (no XQuartz required)
- Terminal output shows detailed simulation events
- Statistics printed every 3 seconds

## Troubleshooting

If graphics don't appear:
1. Ensure you're on macOS with native GLUT support
2. On Linux, install `freeglut3-dev` and update Makefile
3. Check that configuration file is valid

## Author

Multi-threaded apes simulation project
