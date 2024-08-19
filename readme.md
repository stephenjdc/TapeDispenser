# Talking Tape Dispenser

If you'd like to build your own tape dispenser, which tells you how much you've used, and blesses you afterwards, you have two choices.

1. Leave out a pencil and paper one night before bed, and hope that by the morning God will have written down the instructions.
2. Buy one from me. Maybe? Details coming soon(?)
3. Clone this repo, buy the required parts, a 3D printer, and get to work.

Please note that, while the dispenser does aim to fairly accurate, it's not perfectly accurate as it doesn't adjust for the amount of tape used. In the code I've just split the error, so it will tend to overshoot the measurement when a roll is full, and undershoot a bit when it's empty. Given we're only dealing with one-inch resolution, I feel like it's probably fine. You can tweak the code if you want to try and improve on this. There's also is a no-code way to tweak it, detailed below.

## License 
Feel free to make/reuse/modify this project for personal use. In the unlikely event that you're planning to spin up a large-scale production line to make thousands of these, I'd really recommend doing a few more revisions on the code and the models. In which case, I'm available at a very reasonable rate.

## Parts list

### Electronics
- 1× ESP8266 or similar (e.g. I used an ESP-12E)
- 1× IR photodiode (3mm)
- 1× IR LED (3mm)
- 2× 1k resistors
- 2× 10k resistors
- 1× 220Ω resistor
- 1× 2N3904 transistor (or similar)
- 1× LED (colour of your choice, I recommend red)
- 1× 20mm diameter speaker
- 1× 18mm × 12mm slide switch (or any that fits the space on the model)
- Battery clips for 2x AAA batteries

### Dispenser body
#### These parts are 3D-printable
- 1× tape dispenser body
- 1× tape dispenser lid (which holds the spool)
- 1× battery compartment cover
- 1× tape spool
- 1× "Greetings from Kilnettle" text

#### These parts are not 3D printable
- 1× 33mm × 5mm hacksaw blade (for tape cutter)
- 1x 25mm × 1mm steel rod (for example, cut from a large paperclip)
- 2× AAA batteries
- 1x roll of sticky tape with ~27mm inner diameter and ~56mm outer diameter. I used [this one](https://firstclassofficesupplies.com/product/stik-ie-clear-tape-refills/).


## Instructions
I'm gonna keep these fairly sparse, and following what I did. But there's no right way to do it. In the very unlikely event that you actually want to make one yourself, message me with any questions.

1. Flash the ESP-12E with the included sketch, _and data folder_. I used PlatformIO and would suggest you do too.
2. 3D print the included files.
3. Insert the battery clips into the slots in the battery container. Note which side has holes for the positive and negative tabs to slot into.
4. Fit the slider switch into the hole above the battery compartment. Either glue into place, or like I did, use a soldering iron to melt and smush the little nubs to fix it in place.
5. Fit the IR photodiode and LED into the holes on the sides of the lid. They should be a fairly tight fit; push them all the way in. I added a little drop of superglue to keep them fixed in place.
6. Add the speaker. It should click into place at the front of the body, though I added a little hot glue just to be sure. Depending on your soldering skills and exact hardware, it might be easier to add the transistor and resistor on or near the speaker itself, then tuck all that in with the speaker.
7. Solder some wires to the battery tabs and switch.
8. ... follow the rest of the schematic to attach all the parts. Use the thinnest wire you feasibly can, and feel free to carefully route the wires, or just cram things anywhere they'll fit. I'd recommend leaving a decent bit of slack, and regularly trying to fit the lid on to make sure all is fitting well.
9. Once that's done, I'd recommend temporarily connecting the ESP if you can[^1], so you can check the software is flashed properly before you solder anything to it. If all is well, go ahead and solder the ESP.
10. Cover any exposed connections with some tape or hot glue, to prevent shorts.
11. Pop on the lid, and use a little superglue to keep it secure, if you're happy everything is working.
12. Superglue the section of hacksaw blade in place on the front of dispenser. Make sure to put the sharper side facing up, if there is one, so it cuts the tape nicely.
12. Attach the "Greetings from Kilnettle" text to the left side of the dispenser body (left when looking at the bladed end).
13. For the spool, thread the steel rod through the hole in its centre. Add a drop of glue and line it up so that it protrudes an equal amount on either side. It should then press fit into the tape, and you can set it into the dispenser.
    - You may notice there are multiple spools included in the repo. If you've looked at the code, you'll see that the 16-hole spool is the one true spool. In reality, with variances in manufacturing and components, you may want to go for one with fewer holes if you find your dispenser is overshooting the measurements too much.
14. Insert batteries (be careful to put them in the right way round), pop on the battery cover, and all being well, you should now be able to switch it on and enjoy your dispenser!

[^1]: You can easily test it without a spool, by waving your finger around in the spool cavity, making sure you block the IR beam a few times.
