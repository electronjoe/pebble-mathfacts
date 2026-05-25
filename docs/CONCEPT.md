# Concept for MathFacts Pebble Watch App

This document forms the basis of a brainstorming session with Claude Code Superpowers Plugin. It's a seed of the app specification.

The MathFacts App is intended to have approximately the following "day in the life" workflow:

1) MathFacts App is started
2) Presents to the user a historical line graph of fact repetition timings for a specific MathFacts Fact Group
3) Allows use of Up or Down buttons on watch to select between a Fact Group Category for display / trial
3) User presses home button to select a Fact Group Category
4) Present the user a list of Fact Groups within the category
3) Allows use of Up or Down buttons on watch to select between a "fact group" for trial
4) Select button selects fact group, starting a new trial.
5) Trial is executed - depending upon the Fact Group Category, display and button use may differ (see below)
6) Upon completion of the Trial, total elapsed Trial time is added to the On-Pebble database as well as a timestamp
7) Returns user to 1) Home UI

## Definitions

- Fact Group: A set of math facts that are to be drilled in some manner together
- Fact Group Category: A category in which a Fact Group lives.
  - Skip Counting
  - Multiplication
  - Multiplication and Division
- Challenge Question: For Fact Group Categories with per-question challenges
  - e.g. "[?] x 5 = 15"
- Trial: A complete drill of the student on one Fact Group, timed

## Fact Group Categories and Fact Groups

- Skip Counting Category
  - Fact Groups
    - By 2
    - By 3
    - ...
    - By 9
  - How Many: Student will recite orally all skip counts within Fact Group to 100
  - Presentation: Simple text of "Skip Count by [num] to 100..." or similar
  - Control:
    - Mode button press (middle button) signals successful completion
    - Up button press (with some "cancel" text?) signals cancellation (stop timer and return to Home screen of App)
- Multiplcation Category
  - Fact Groups
    - Multiplication of [2-3] x [1-10]
    - Multiplication of [4-5] x [1-10]
    - Multiplication of [6-7] x [1-10]
    - Multiplication of [8-9] x [1-10]
  - How Many: Student will be exposed to 20 Challenge Questions, randomized within the Fact Group
  - Presentation: randomized format, one of four formats for Challenge Question
    - [?] x 2 = 6
    - 2 x [?] = 6
    - 6 = [?] x 2
    - 6 = 2 x [?]
  - Control:
    - Mode button press: Successfully answered the Challenge Question, move to next
    - Up button press (with some "cancel" text?) signals cancellation (stop timer and return to Home screen of App)
    - Down button press (with "skip" text) signals skipping the Challenge Question (but it must still be retained in the challenge list
      - E.g. if doing a Multiplication Fact Group Trial with 10 questions, and the student skips the 3rd posed, that now becomes 10th
- Multiplication & Division Category
  - Same as Multiplication Category but randomizes both types 50/50
  - Presentation:
    - Mix in e.g. "[?] / 3 = 5" style division questions, all four randomized formats just like multiplication

## UI Descriptions (Flexible, Suggestions)

### Home UI

- When the app first starts, it displays the home UI
- MathFacts application name somewhere
- Fact Group Category labeled somewhere (which is displayed on the graph)
- Display is dominantly a Graph - one line graph per Fact Group (Within selected category)
  - Line graph domain is days, beginning at earliest data point in Trial time database on watch, or 120 days ago, whichover is more recent
  - Line graph range begins at best time of the trial database, ends at worst time on the trial database (for tha Fact Group)
  - Line segment between the best performance every day (so if more than one Trial for Fact Group in a day, take best) - e.g. only one data point per day (or less)
- Up and Down Button rotate through Fact Group Cateogries
- Select Button selects a Fact Group Category and moves to Fact Group Selection UI
- Pressing and HOLDING the Mode button for more than 5 seconds activates the Clear Data UI

### Clear Data UI

- Warns user that all data will be lost
- Pesents user with "Cancel" - Up Button
- Presents user with "Clear all Data" - Down Button
- Either way (Up or Down) Returns to  Home UI

### Fact Group Selection UI

- Presents a list of Fact Gruops available for selection
  - If too long for display, allow scroll off bottom / off top for when user hits Up / Down buttons and goes off display
- Select button selects the Fact Group and begins Challenge UI for that Fact Group Category / Fact Group

### Challenge UI

- When any Challenge Completes, move on to Challenge Summary UI

#### Skip Counting

- Display just the challenge text "Skip count from 5 to 100"
- Home button press indicates completion (note visually somehow)

#### Multilpication / Mult & Div Fact Groups

- Display the randomized challenge text
- Home button press indicates success of the fact (note visuall somehow)
- Down button press skips this particular challenge question

### Challenge Summary UI

- Display this Trial's timing relative to last three trials
- Offer two display options selectable by Up / Down and Select
- "Store Trial" to store the result to the database
- "Discard Trial" to discard the result (do not store to database)
- Either way, return to Home UI after selection

### Multiplication 

## Constraints

- Target only Pebble Flint smartwatch for now (Pebble Time Duo)
- Keep in mind when doing UI design the small screen resolution

## Process

- I do want superpowers brainstorming to use the new "website hosted locally for UI mocks" ability

## Resources

- Note Pebble SDK and related resources are available to you at the repo's `resources/` directory, start with `resources/CLAUDE.md`
