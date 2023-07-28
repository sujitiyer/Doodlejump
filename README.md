**Doodle Jump Spin-Off | Caltech CS 3 Final Project | June 2023**

_Sujit Iyer, Garrett Knuf, James Downs, Thorsen Kristufek_


Doodle Jump is a vertical-scrolling, platform-based game. The player controls an alien with the left and right arrow keys as the alien jumps between platforms. Collisions with the randomly spawned platforms will only occur when the alien lands on top of the platform (i.e., platform collisions will not occur when the alien’s velocity is upwards). As the alien climbs the screen, the screen scrolls vertically to show higher up platforms. The vertical scrolling is unidirectional, meaning the display will never scroll downwards after the alien makes upwards progress. Moving the alien off of the left or right edge will wrap the alien around the other edge of the screen. The alien should never be able to jump past the top of the screen since upwards movement will be handled with vertical-scrolling.

The game ends when the alien hits the bottom of the screen, collides with a monster, or gets sucked into a blackhole. Monsters are randomly spawned on platforms, and they can be killed if they are shot by the alien. A user can press keys to fire a shot at a monster using preset shooting angles. If a monster is killed, it will disappear, allowing the alien to jump onto the platform it was previously located. Black holes are also randomly spawned on the background of the scene. If the alien is moved within the radius of the blackhole, the game will also end.

There is no winning condition since the objective of the game is to score as many points as possible. Points can be earned by causing the scene to scroll vertically, revealing higher elevated platforms. Points can also be earned by killing monsters with bullets or by collecting power ups. The possible power ups are springs and jetpacks which are placed on platforms and accelerate the player upwards at faster rates. Springs apply an instant boost in upwards velocity while jetpacks provide a constant velocity upwards for a few seconds.


