Storage 
{
	{ Format 1.33 }
	{ GeneratedFrom TSCD-version-2.20 }
	{ WrittenBy cuse }
	{ WrittenOn "" }
}

Document 
{
	{ Type "Statechart Diagram" }
	{ Name EGADSR.scd }
	{ Author cuse }
	{ CreatedOn "" }
	{ Annotation "" }
	{ Hierarchy True }
}

Page 
{
	{ PageOrientation Portrait }
	{ PageSize A4 }
	{ ShowHeaders False }
	{ ShowFooters False }
	{ ShowNumbers False }
}

Scale 
{
	{ ScaleValue 1 }
}

# GRAPH NODES

SCDOrState 1
{
	{ Name "Attack\r\rdo: raise level lin.\rdo: count time" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

SCDOrState 2
{
	{ Name "Attack_Hold\r\rdo: hold level" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

SCDOrState 3
{
	{ Name "Decay_1\r\rdo: reduce level exp.\rdo: count time" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

SCDOrState 4
{
	{ Name "Decay_2\r\rdo: reduce level exp." }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

SCDOrState 5
{
	{ Name "Sustain\r\rdo: hold level" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

SCDOrState 6
{
	{ Name "Release\r\rdo: reduce level exp." }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

SCDFinalState 7
{
	{ Name "" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

SDCDefaultState 8
{
	{ Name "" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

Comment 40
{
	{ Name "gig::Engine\rADSR Envelope Generator (EG1 = Amplitude EG, EG2 = Filter Cutoff Freq. EG)" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

SCDFinalState 48
{
	{ Name "" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

SCDFinalState 49
{
	{ Name "" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

SCDFinalState 50
{
	{ Name "" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

SCDFinalState 51
{
	{ Name "" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

# GRAPH EDGES

SCDTransitionEdge 9
{
	{ Name "Voice triggered\r/ set level = PreAttack" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 8 }
	{ Subject2 1 }
}

SCDTransitionEdge 10
{
	{ Name "Attack time reached\r[AttackHold flag set]" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 1 }
	{ Subject2 2 }
}

SCDTransitionEdge 11
{
	{ Name "Attack time reached\r[AttackHold flag not set]" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 1 }
	{ Subject2 3 }
}

SCDTransitionEdge 12
{
	{ Name "Playback reached Loop start" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 2 }
	{ Subject2 3 }
}

SCDTransitionEdge 13
{
	{ Name "Decay_1 time reached\r[InfiniteSustain flag not set]" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 3 }
	{ Subject2 4 }
}

SCDTransitionEdge 14
{
	{ Name "Decay_1 time reached\r[InfiniteSustain flag set]" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 3 }
	{ Subject2 5 }
}

SCDTransitionEdge 15
{
	{ Name "Voice released" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 5 }
	{ Subject2 6 }
}

SCDTransitionEdge 16
{
	{ Name "Voice released" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 4 }
	{ Subject2 6 }
}

SCDTransitionEdge 17
{
	{ Name "level < min. level" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 6 }
	{ Subject2 7 }
}

SCDTransitionEdge 36
{
	{ Name "Release cancelled\r[InfiniteSustain flag set]" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 6 }
	{ Subject2 5 }
}

SCDTransitionEdge 37
{
	{ Name "Release cancelled\r[InfiniteSustain flag not set]" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 6 }
	{ Subject2 4 }
}

SCDTransitionEdge 42
{
	{ Name "level < min. level" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 4 }
	{ Subject2 7 }
}

SCDTransitionEdge 44
{
	{ Name "Voice released" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 2 }
	{ Subject2 6 }
}

SCDTransitionEdge 45
{
	{ Name "Voice released" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 3 }
	{ Subject2 6 }
}

SCDTransitionEdge 52
{
	{ Name "Killed" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 1 }
	{ Subject2 48 }
}

SCDTransitionEdge 53
{
	{ Name "Killed" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 3 }
	{ Subject2 48 }
}

SCDTransitionEdge 54
{
	{ Name "Killed" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 2 }
	{ Subject2 49 }
}

SCDTransitionEdge 55
{
	{ Name "Killed" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 5 }
	{ Subject2 50 }
}

SCDTransitionEdge 56
{
	{ Name "Killed" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 6 }
	{ Subject2 50 }
}

SCDTransitionEdge 57
{
	{ Name "Killed" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 4 }
	{ Subject2 51 }
}

# VIEWS AND GRAPHICAL SHAPES

View 18
{
	{ Index "0" }
	{ Parent 0 }
}

RoundedBox 19
{
	{ View 18 }
	{ Subject 1 }
	{ Position 230 190 }
	{ Size 193 76 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

RoundedBox 20
{
	{ View 18 }
	{ Subject 2 }
	{ Position 580 190 }
	{ Size 130 58 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

RoundedBox 21
{
	{ View 18 }
	{ Subject 3 }
	{ Position 230 330 }
	{ Size 202 76 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

RoundedBox 22
{
	{ View 18 }
	{ Subject 4 }
	{ Position 530 460 }
	{ Size 193 58 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

RoundedBox 23
{
	{ View 18 }
	{ Subject 5 }
	{ Position 90 460 }
	{ Size 130 58 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

RoundedBox 24
{
	{ View 18 }
	{ Subject 6 }
	{ Position 310 620 }
	{ Size 193 58 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

BullsEye 25
{
	{ View 18 }
	{ Subject 7 }
	{ Position 310 710 }
	{ Size 20 20 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName True }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

BlackDot 26
{
	{ View 18 }
	{ Subject 8 }
	{ Position 230 90 }
	{ Size 8 8 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName True }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 27
{
	{ View 18 }
	{ Subject 9 }
	{ FromShape 26 }
	{ ToShape 19 }
	{ Curved False }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 2 }
	{ Point 230 94 }
	{ Point 230 152 }
	{ NamePosition 216 123 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 28
{
	{ View 18 }
	{ Subject 10 }
	{ FromShape 19 }
	{ ToShape 20 }
	{ Curved False }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 2 }
	{ Point 326 190 }
	{ Point 515 190 }
	{ NamePosition 424 190 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 29
{
	{ View 18 }
	{ Subject 11 }
	{ FromShape 19 }
	{ ToShape 21 }
	{ Curved False }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 2 }
	{ Point 230 228 }
	{ Point 230 292 }
	{ NamePosition 216 260 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 30
{
	{ View 18 }
	{ Subject 12 }
	{ FromShape 20 }
	{ ToShape 21 }
	{ Curved True }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 4 }
	{ Point 550 219 }
	{ Point 550 280 }
	{ Point 460 300 }
	{ Point 331 317 }
	{ NamePosition 479 285 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 31
{
	{ View 18 }
	{ Subject 13 }
	{ FromShape 21 }
	{ ToShape 22 }
	{ Curved True }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 4 }
	{ Point 331 340 }
	{ Point 450 340 }
	{ Point 520 380 }
	{ Point 520 431 }
	{ NamePosition 465 371 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 32
{
	{ View 18 }
	{ Subject 14 }
	{ FromShape 21 }
	{ ToShape 23 }
	{ Curved True }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 4 }
	{ Point 129 340 }
	{ Point 100 340 }
	{ Point 70 390 }
	{ Point 70 431 }
	{ NamePosition 118 385 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 33
{
	{ View 18 }
	{ Subject 15 }
	{ FromShape 23 }
	{ ToShape 24 }
	{ Curved True }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 4 }
	{ Point 60 489 }
	{ Point 60 560 }
	{ Point 120 630 }
	{ Point 214 630 }
	{ NamePosition 116 588 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 34
{
	{ View 18 }
	{ Subject 16 }
	{ FromShape 22 }
	{ ToShape 24 }
	{ Curved True }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 4 }
	{ Point 540 489 }
	{ Point 540 550 }
	{ Point 480 610 }
	{ Point 407 610 }
	{ NamePosition 497 581 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 35
{
	{ View 18 }
	{ Subject 17 }
	{ FromShape 24 }
	{ ToShape 25 }
	{ Curved False }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 2 }
	{ Point 310 649 }
	{ Point 310 700 }
	{ NamePosition 329 676 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 38
{
	{ View 18 }
	{ Subject 36 }
	{ FromShape 24 }
	{ ToShape 23 }
	{ Curved True }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 4 }
	{ Point 270 591 }
	{ Point 270 550 }
	{ Point 220 470 }
	{ Point 155 470 }
	{ NamePosition 197 507 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 39
{
	{ View 18 }
	{ Subject 37 }
	{ FromShape 24 }
	{ ToShape 22 }
	{ Curved True }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 4 }
	{ Point 360 591 }
	{ Point 360 540 }
	{ Point 390 480 }
	{ Point 434 474 }
	{ NamePosition 404 533 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

TextBox 41
{
	{ View 18 }
	{ Subject 40 }
	{ Position 300 40 }
	{ Size 20 20 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-courier-bold-r-normal--8*" }
	{ TextAlignment Left }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 43
{
	{ View 18 }
	{ Subject 42 }
	{ FromShape 22 }
	{ ToShape 25 }
	{ Curved True }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 4 }
	{ Point 580 489 }
	{ Point 580 630 }
	{ Point 430 700 }
	{ Point 320 709 }
	{ NamePosition 485 661 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 46
{
	{ View 18 }
	{ Subject 44 }
	{ FromShape 20 }
	{ ToShape 24 }
	{ Curved True }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 4 }
	{ Point 586 219 }
	{ Point 830 820 }
	{ Point 190 820 }
	{ Point 293 649 }
	{ NamePosition 584 630 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 47
{
	{ View 18 }
	{ Subject 45 }
	{ FromShape 21 }
	{ ToShape 24 }
	{ Curved True }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 4 }
	{ Point 290 368 }
	{ Point 290 410 }
	{ Point 310 480 }
	{ Point 310 591 }
	{ NamePosition 301 462 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

BullsEye 58
{
	{ View 18 }
	{ Subject 48 }
	{ Position 30 230 }
	{ Size 20 20 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName True }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 59
{
	{ View 18 }
	{ Subject 52 }
	{ FromShape 19 }
	{ ToShape 58 }
	{ Curved True }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 4 }
	{ Point 134 170 }
	{ Point 90 170 }
	{ Point 50 180 }
	{ Point 34 221 }
	{ NamePosition 57 173 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 60
{
	{ View 18 }
	{ Subject 53 }
	{ FromShape 21 }
	{ ToShape 58 }
	{ Curved True }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 4 }
	{ Point 129 310 }
	{ Point 70 310 }
	{ Point 30 280 }
	{ Point 30 240 }
	{ NamePosition 49 301 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

BullsEye 61
{
	{ View 18 }
	{ Subject 49 }
	{ Position 580 90 }
	{ Size 20 20 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName True }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 62
{
	{ View 18 }
	{ Subject 54 }
	{ FromShape 20 }
	{ ToShape 61 }
	{ Curved False }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 2 }
	{ Point 580 161 }
	{ Point 580 100 }
	{ NamePosition 579 132 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

BullsEye 63
{
	{ View 18 }
	{ Subject 50 }
	{ Position 80 690 }
	{ Size 20 20 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName True }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 64
{
	{ View 18 }
	{ Subject 55 }
	{ FromShape 23 }
	{ ToShape 63 }
	{ Curved True }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 4 }
	{ Point 42 489 }
	{ Point 30 550 }
	{ Point 30 610 }
	{ Point 75 682 }
	{ NamePosition 36 572 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 65
{
	{ View 18 }
	{ Subject 56 }
	{ FromShape 24 }
	{ ToShape 63 }
	{ Curved True }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 4 }
	{ Point 242 649 }
	{ Point 220 680 }
	{ Point 140 700 }
	{ Point 90 690 }
	{ NamePosition 159 679 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

BullsEye 66
{
	{ View 18 }
	{ Subject 51 }
	{ Position 380 400 }
	{ Size 20 20 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName True }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 67
{
	{ View 18 }
	{ Subject 57 }
	{ FromShape 22 }
	{ ToShape 66 }
	{ Curved True }
	{ End1 Empty }
	{ End2 FilledArrow }
	{ Points 4 }
	{ Point 434 444 }
	{ Point 410 440 }
	{ Point 390 430 }
	{ Point 383 409 }
	{ NamePosition 419 421 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-courier-medium-r-normal--8*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

