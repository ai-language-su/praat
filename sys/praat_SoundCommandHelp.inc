/* Sound action help, reviewed against registration callbacks and bundled manual.
   The short summary must explain the result without requiring the long guide.
   Each record applies only to Sound-only selections, never to a namesake command
   on Spectrum, Pitch, or another object type. See docs/SOUND_TOOLTIP_AUDIT.md. */

SOUND_HELP(U"Sound help", U"Open Praat's manual page explaining Sound objects and the operations available for recordings.",
 U"What is a Sound in Praat? An object holding a sampled waveform: regularly spaced amplitude values for one or more audio channels. It is loaded in memory; changing it does not automatically update a disk file.\n"
 U"What happens when I click? The manual opens. Use it for details and links to Sound commands; this button does not play or analyse the recording.")
SOUND_HELP(U"View & Edit", U"Open the Sound editor to listen, zoom, select time intervals, inspect analyses and edit the selected recording.",
 U"What opens in Praat? A separate editor for this Sound. Its waveform plots amplitude against time, and optional analysis panels show a spectrogram, pitch, formants or intensity. Those displays analyse the Sound; showing them does not publish separate analysis objects in Objects.\n"
 U"Why use an editor? The Objects window manages whole recordings and analysis results. The editor lets you work at a particular time or interval: listen closely, zoom in, compare analyses and make audio edits.\n"
 U"How do I use it? Click or drag on the time display to select a moment or stretch, then use the editor's playback controls or menus. Editing samples changes this Sound in memory; use Copy first if you need a separate original. Save or export the result when a disk file is needed. Closing the editor does not remove the object from Objects.")
SOUND_HELP(U"Play", nullptr, nullptr)
SOUND_HELP(U"Draw...", U"Draw the Sound's waveform in the Picture window, using chosen time and amplitude ranges; the recording stays unchanged.",
 U"What does the waveform show? Signed sample amplitude versus time. Its positive and negative swings are not pitch or a loudness curve. A strong waveform can have low or high pitch.\n"
 U"How do I use it? Set time and amplitude limits, a drawing method such as curve or bars, and whether to include labels and axes (Garnish). The figure appears in the Picture window for layout or export. Use View & Edit for interactive listening and editing rather than preparing a static figure.")
SOUND_HELP(U"Draw where...", U"Draw only the waveform portions satisfying a formula, in the Picture window; leave the Sound unchanged.",
 U"What does where mean? A formula decides which positions qualify for drawing. For a Sound, x is time in seconds and self is the sample amplitude. For example, self > 0 selects positive amplitudes.\n"
 U"How do I use it? Set the time and amplitude ranges and the condition, then view the Picture window. This controls a figure; it does not cut samples out of the recording.")
SOUND_HELP(U"Paint where...", U"Fill the area between a waveform and a chosen level wherever a formula condition is true, in the Picture window.",
 U"What does paint do? It shades an area under or above the waveform relative to a horizontal level. The condition determines where the fill is applied; the time and amplitude limits define the displayed region.\n"
 U"How do I use it? Choose the channel, drawing region, colour, reference level and formula. A formula such as self > 0 tests sample amplitude; x refers to time. Inspect the Picture figure. The audio samples are unchanged.")
SOUND_HELP(U"Paint enclosed...", U"Shade the region between two selected Sound waveforms in the Picture window, without combining or modifying their audio.",
 U"What is enclosed? The space between the two curves when plotted on the same time and amplitude axes. It is a visual comparison, not a noise-removal or audio-mixing operation.\n"
 U"How do I use it? Select two Sounds, review time range, vertical range and colour, then inspect the Picture window. Choose comparable time spans so that the shaded difference is meaningful.")
SOUND_HELP(U"Get number of channels", U"Report how many audio channels the selected Sound contains: one for mono, two for stereo, or more for multichannel audio.",
 U"What is a channel? One stream of waveform samples. Stereo commonly has left and right streams. Two channels do not necessarily mean two speakers, and two selected Sound objects are not the same as two channels inside one Sound.\n"
 U"How do I use it? Select one Sound and read the count in Info. This helps choose a channel for playback, extraction or measurement; it changes nothing.")
SOUND_HELP(U"Get number of samples", U"Report the number of stored sample positions per channel in the Sound, not the number of words, pulses, or all-channel values combined.",
 U"What is a sample? One stored amplitude measurement. Samples are evenly spaced in time. At 44100 Hz sampling frequency, one second normally contains 44100 samples per channel.\n"
 U"How do I use it? Select one Sound and read Info. Combine the count with the sampling period to understand the physical duration; stereo has two amplitude values at each sample position.")
SOUND_HELP(U"Get sampling period", U"Report the time in seconds between consecutive Sound samples: the reciprocal of its sampling frequency.",
 U"What is the sampling period? The interval between stored waveform values, not the period of a speaker's vocal-fold vibration. At 10000 samples per second it is 0.0001 seconds.\n"
 U"How do I use it? Select one Sound and read Info. Smaller periods mean finer time sampling. This query reports the existing spacing and does not resample the audio.")
SOUND_HELP(U"Get sampling frequency", U"Report the Sound's sampling frequency in Hz: how many samples per second are stored in each channel.",
 U"How is this different from pitch? Sampling frequency describes the recording's time grid, such as 44100 Hz. Pitch describes repetition in the sound, such as vocal-fold vibration around 150 Hz. They are different quantities.\n"
 U"Why check it? It affects which frequencies a recording can represent and whether Sounds can be combined. Select one Sound and read Info; use Resample to create audio on another sampling grid.")
SOUND_HELP(U"Get time from sample number...", U"Convert a sample position into its time in seconds on the Sound's timeline.",
 U"How does numbering work? Sample number 1 is the first stored position, usually centred half a sampling period after the Sound's start boundary. A sample number is an index, not milliseconds.\n"
 U"How do I use it? Select one Sound, enter a sample number and read the calculated time in Info. The conversion uses the Sound's current starting sample time and sampling period; it does not move the audio.")
SOUND_HELP(U"List all sample times", U"List the times in seconds of every stored sample position in the selected Sound.",
 U"What is listed? The time grid, not the sample amplitudes. All channels of a Sound share those times. The output can be very large for a long or highly sampled recording.\n"
 U"How do I use it? Select one Sound and inspect the list in Info. Use Get time from sample number for a single position, or Get sampling period for the interval between positions.")
SOUND_HELP(U"Get sample number from time...", U"Convert a time in seconds into a position on the Sound's sample grid; the result may be fractional.",
 U"Why can the result be fractional? A requested time can fall between two stored samples. The grid starts at sample 1; a result of 20.5 lies halfway between positions 20 and 21.\n"
 U"How do I use it? Enter a time and read Info. Check that it lies within the Sound before using the result as a stored sample index. This is a time-to-position conversion, not a search for a pulse or word.")
SOUND_HELP(U"Get value at time...", U"Report the waveform amplitude at a chosen time and channel, estimating between stored samples when necessary.",
 U"What is the value? A signed waveform amplitude, displayed in pascals (Pa). Positive and negative values describe opposite pressure deviations; they are not pitch or loudness. Physical pressure interpretation requires calibrated audio.\n"
 U"What is interpolation? A way to estimate a value between samples. Different methods can give different estimates. Choose the channel, time in seconds and interpolation, then read Info. The Sound is unchanged.")
SOUND_HELP(U"Get value at sample number...", U"Read the stored waveform amplitude at a chosen sample index and channel.",
 U"What am I reading? One signed amplitude value on the recording's time grid, not the strength of an entire word. Praat displays amplitude in Pa; physical pressure depends on recording calibration.\n"
 U"How do I use it? Choose the channel and an integer sample number, starting at 1. Use Get number of samples to check the upper limit and Get time from sample number to locate that position in seconds. Read the value in Info.")
SOUND_HELP(U"Get minimum...", U"Report the lowest signed waveform amplitude within a chosen time range of the Sound.",
 U"What does minimum mean? The deepest negative waveform value. It is not the quietest moment: a large negative amplitude can be part of a strong signal. Interpolation estimates peaks between samples.\n"
 U"How do I use it? Select one Sound, set a time range and interpolation, then read Info. Compare with Get maximum or Get absolute extremum for the other peaks. Calibrated pressure amplitudes are in Pa.")
SOUND_HELP(U"Get maximum...", U"Report the highest signed waveform amplitude within a chosen time range of the Sound.",
 U"What does maximum mean? The most positive waveform value, not the highest pitch or the greatest averaged intensity. Interpolation can locate a peak between stored samples.\n"
 U"How do I use it? Select one Sound, set the time range and interpolation and read Info. Use Get time of maximum to find when it occurs. Pressure units are Pa when the recording is calibrated.")
SOUND_HELP(U"Get time of minimum...", U"Report the time in seconds where the Sound reaches its lowest signed amplitude in a chosen range.",
 U"Which minimum? The most negative waveform peak, not a pause or a minimum in the pitch curve. Interpolation estimates its position between samples.\n"
 U"How do I use it? Set a time range and interpolation, then read Info and inspect that time in View & Edit. Use Get minimum for the amplitude value itself. No samples are changed.")
SOUND_HELP(U"Get time of maximum...", U"Report the time in seconds where the Sound reaches its highest signed amplitude in a chosen range.",
 U"Which maximum? The largest positive waveform peak, not the loudest syllable averaged across time. Interpolation can place the estimated peak between sample positions.\n"
 U"How do I use it? Set a time range and interpolation, read Info, and locate the reported time in the Sound editor. Use Get maximum to get the amplitude instead of its time.")
SOUND_HELP(U"Get absolute extremum...", U"Report the largest waveform amplitude magnitude in a chosen range, considering both positive and negative peaks.",
 U"What does absolute mean? Ignore the sign when comparing peaks: a value of -0.8 has a larger magnitude than +0.5. This measures a waveform peak, not average intensity.\n"
 U"How do I use it? Set the time range and interpolation and read Info. This is useful when checking peak size before scaling. Use root-mean-square or intensity for a time-averaged level.")
SOUND_HELP(U"Get nearest zero crossing...", U"Find the time where a chosen Sound channel crosses amplitude zero nearest to a specified time.",
 U"What is a zero crossing? A change from negative to positive waveform amplitude or the reverse. It can help choose a cut near zero, although that alone does not guarantee an inaudible join. Noise can cause many crossings.\n"
 U"How do I use it? Choose a channel and a time in seconds and read the crossing time in Info. This reports a position; it does not cut or edit the waveform.")
SOUND_HELP(U"Get nearest level crossing...", U"Find the time where the waveform crosses a specified amplitude level nearest to a chosen time.",
 U"What is a level crossing? A place where the waveform passes through an amplitude value you specify. It is not automatically a threshold on averaged intensity, nor a word boundary.\n"
 U"How do I use it? Choose the channel, reference time, amplitude level and search direction (which side of the reference time to search). Read the crossing time in Info and check it against the waveform. For amplitude zero, Get nearest zero crossing is the simpler query.")
SOUND_HELP(U"Get mean...", U"Report the average signed waveform amplitude for a chosen channel and time range, useful for checking a DC offset.",
 U"What is a DC offset? A persistent shift of the waveform away from zero. Positive and negative oscillations can cancel in the signed mean, so a mean near zero does not mean the recording is silent.\n"
 U"How do I use it? Choose the channel and time range and read Info. Use Subtract mean to remove the offset, and root-mean-square or intensity to measure signal strength. Amplitude is displayed in Pa; physical units require calibration.")
SOUND_HELP(U"Get root-mean-square...", U"Report root-mean-square (RMS) amplitude: a time-averaged measure of waveform strength that counts positive and negative values equally.",
 U"What is RMS? Square each amplitude, average those squares, then take the square root. Unlike the signed mean, this does not cancel positive against negative oscillations. It has the same units as amplitude, displayed as Pa.\n"
 U"How do I use it? Set a time range and read Info. RMS includes any DC offset. It is not peak amplitude or perceived loudness, and physical pressure comparisons require calibrated, comparable recordings.")
SOUND_HELP(U"Get standard deviation...", U"Report how widely waveform amplitudes vary around their mean in a chosen time range.",
 U"What is standard deviation? A measure of spread: larger values mean more variation in amplitude. Unlike RMS, it measures departures from the mean, so a constant offset is treated differently. It is not variation in pitch or timing.\n"
 U"How do I use it? Choose the range and any channel settings in the dialog, then read Info. The value has amplitude units, displayed as Pa. It does not change the recording.")
SOUND_HELP(U"Get energy...", U"Report the time integral of squared Sound amplitude over a chosen range; longer signals can have more energy at the same average level.",
 U"What does energy mean here? Praat sums squared amplitude across time. Its units are Pa squared times seconds for calibrated pressure, not joules. This includes both signal strength and duration.\n"
 U"How do I use it? Select a time range and read Info. Compare with Get power, which divides out duration. In multichannel Sounds Praat averages squared amplitudes across channels rather than simply adding them. Use Get energy in air for Praat's physical acoustic-energy estimate.")
SOUND_HELP(U"Get power...", U"Report mean squared Sound amplitude over a chosen time range, a measure of average signal strength rather than accumulated energy.",
 U"What is power here? Squared amplitude averaged over time, in Pa squared if amplitudes are calibrated pressure. It is energy divided by the interval's duration, not electrical power in watts or perceived loudness.\n"
 U"How do I use it? Set the time range and read Info. Praat averages across channels for this measurement. Use Get power in air for an acoustic-power-per-area estimate and Get intensity (dB) for a logarithmic level.")
SOUND_HELP(U"Get energy in air", U"Estimate acoustic energy per unit area over the whole Sound, expressed in joules per square metre, assuming calibrated pressure amplitudes.",
 U"What is assumed? Praat treats the samples as sound pressure in Pa and uses an air-density and sound-speed conversion. Ordinary uncalibrated recording levels cannot establish an absolute physical exposure.\n"
 U"How do I use it? Select one Sound and read Info. This estimate includes duration; Get power in air gives the corresponding time-averaged quantity. It does not measure a loudspeaker's total energy or change the audio.")
SOUND_HELP(U"Get power in air", U"Estimate average acoustic power per unit area over the whole Sound, in watts per square metre, assuming calibrated pressure amplitudes.",
 U"What does per unit area mean? Energy flow through an area, not a loudspeaker's electrical power. Praat derives it from mean squared pressure using assumptions about air. Absolute physical interpretation requires calibrated amplitudes in Pa.\n"
 U"How do I use it? Select one Sound and read Info. Compare with Get energy in air, which also depends on duration, or Get intensity (dB), which uses a logarithmic scale.")
SOUND_HELP(U"Get intensity (dB)", U"Report one overall logarithmic level for the whole Sound, in decibels relative to 20 micropascals; absolute sound-pressure interpretation requires calibration.",
 U"What are decibels? A logarithmic way to express a ratio. Here the calculation uses average squared pressure relative to a reference pressure of 20 micropascals. A louder recording level changes the result even if the original speaker was unchanged.\n"
 U"How do I use it? Select one Sound and read Info. This returns a whole-Sound value; To Intensity creates a curve over time. Neither gives perceived loudness directly, and an ordinary audio file is not automatically a calibrated sound-level measurement.")

SOUND_HELP(U"Reverse", U"Reverse the sample order in each selected Sound so it plays backward; modify the existing Sound.",
 U"What changes? The end of the recording is heard first and the beginning last. The sample values are reordered; this is different from flipping their positive and negative signs.\n"
 U"How do I use it? Make a Copy if you want to keep both versions, then choose Reverse and play the selected Sound. This edits the object in memory; saving to a file is separate.")
SOUND_HELP(U"Formula...", U"Replace the selected Sound's sample values with the result of a formula, changing the Sound itself.",
 U"What is a formula here? An expression evaluated at the waveform's sample positions. In a Sound formula, x is time in seconds, self is the current sample amplitude and row identifies the channel. For example, self * 0.5 halves amplitudes; 0 replaces them with silence.\n"
 U"Why use it? Apply a precise mathematical change that is not covered by a ready-made command. The same words have different coordinates in other object types: x is time here, not frequency.\n"
 U"How do I use it? Copy the Sound before experimenting, enter the expression and inspect or play the changed Sound. A formula can replace all sample data, so check its meaning before confirming.")
SOUND_HELP(U"Formula (part)...", U"Apply a sample-value formula only to the specified time and channel region of the existing Sound.",
 U"What is different from Formula? The selected time and channel range limits which samples are replaced. In the expression, x is time in seconds and self is the sample's current amplitude.\n"
 U"How do I use it? Copy the Sound if needed, choose the time span and channels, and enter a formula such as self * 0.5 to reduce amplitudes there. Listen around both boundaries: abrupt changes can introduce clicks. This changes audio data in the same object.")
SOUND_HELP(U"Add...", U"Add a constant to every sample amplitude in the selected Sound, shifting the waveform's baseline.",
 U"What does adding do? It introduces or changes a DC offset, a constant displacement from amplitude zero. It does not append audio, mix another recording, or simply turn up the sound.\n"
 U"How do I use it? Enter an amplitude offset and inspect the changed waveform. Use Multiply or Scale peak for an overall gain change, and Subtract mean to remove an average offset. Copy the Sound first if the original is needed.")
SOUND_HELP(U"Subtract mean", U"Remove each channel's average amplitude from the selected Sound, centring its waveform around zero.",
 U"Why remove the mean? A DC offset shifts a waveform's baseline even though it is not an ordinary audible oscillation. Subtracting the mean can prepare it for analysis or processing. It does not remove background noise or subtract another Sound.\n"
 U"How do I use it? Select the Sound and run the command. The existing samples change. Use Get mean to inspect the offset and keep a Copy if you want the original values.")
SOUND_HELP(U"Multiply...", U"Multiply every sample amplitude by a factor, changing the selected Sound's gain in place.",
 U"What does the factor mean? A factor of 0.5 halves amplitudes, 2 doubles them, 0 makes silence and -1 reverses polarity. Polarity reversal changes signs, not playback direction.\n"
 U"How do I use it? Choose a factor and inspect or play the changed Sound. This is linear amplitude scaling, not setting a target dB level. Very large peaks may clip during playback or file export; Scale peak sets a chosen peak instead.")
SOUND_HELP(U"Multiply by window...", U"Multiply the existing waveform by a chosen weighting shape, usually reducing its edges before analysis or extraction.",
 U"What is a window? A series of weights across time. A tapered window reduces the start and end gradually, helping avoid abrupt edges in spectral analysis. It is not a new editor window and does not filter by a chosen frequency band.\n"
 U"How do I use it? Copy the Sound if needed, choose a window shape and inspect its effect on the waveform. The same Sound's samples change and the edges can become quieter. Use a rectangular window when no tapering is intended.")
SOUND_HELP(U"Scale peak...", U"Scale the Sound's amplitudes so its largest absolute peak reaches the value you choose; modify the existing Sound.",
 U"What is an absolute peak? The largest amplitude magnitude, considering both positive and negative values. Scaling multiplies the waveform uniformly; it does not even out loud and quiet sections.\n"
 U"How do I use it? Choose a target peak and compare the changed Sound with a copy. This is peak normalisation, not a target for perceived loudness or intensity in dB. Peaks near standard playback limits leave less room for further processing.")
SOUND_HELP(U"Scale intensity...", U"Multiply the Sound's amplitudes to reach a chosen overall intensity in dB, changing the existing Sound.",
 U"What level is being set? Praat's mean-square-pressure level, expressed relative to 20 micropascals. It scales the whole waveform uniformly; quiet and loud parts keep their relative differences. The resulting physical sound-pressure level is meaningful only with appropriate calibration.\n"
 U"How do I use it? Set the target intensity and inspect peak amplitude afterward. A target averaged level can still produce large peaks that clip in playback or export. This differs from Scale peak, which controls the largest amplitude rather than the average level.")
SOUND_HELP(U"Set value at sample number...", U"Replace one stored amplitude value at a chosen channel and sample position in the existing Sound.",
 U"What is changed? One waveform sample, not a whole word, time interval or pitch estimate. A sample number is an integer index starting at 1. Abrupt changes relative to neighbouring samples can produce clicks.\n"
 U"How do I use it? Check the channel and sample number, choose the new amplitude and confirm. Inspect the surrounding waveform. Use Get time from sample number to locate it, or Formula (part) for a region.")
SOUND_HELP(U"Set part to zero...", U"Replace waveform amplitudes in a chosen time interval with zero, keeping the interval's duration in the Sound.",
 U"What does zero do? It makes digital silence in that interval. It does not delete the time span or move later audio earlier. Settings can move boundaries toward nearby zero crossings to reduce abrupt transitions.\n"
 U"How do I use it? Copy the Sound if needed, set the time range and boundary options, then inspect and listen around the changed region. Zero crossings do not guarantee a smooth join for every signal.")
SOUND_HELP(U"Override sampling frequency...", U"Change the Sound's declared sampling frequency without recalculating sample values; playback duration and pitch change.",
 U"Why is this different from Resample? The same samples are interpreted as occurring faster or slower. Doubling the declared rate plays them in half the time with frequencies doubled. Resample instead computes a new sample grid while preserving timing and pitch.\n"
 U"When would I use it? Correct a recording whose sampling frequency was labelled incorrectly, or deliberately reinterpret its speed. Enter the intended frequency in Hz and check duration and playback. This changes the existing Sound; it is not a general audio-quality upgrade.")
SOUND_HELP(U"Get start time", U"Report the Sound object's start boundary in seconds; it need not be zero or the centre time of the first sample.",
 U"What does the boundary mean? It marks the beginning of this object's time domain. Extracted or shifted Sounds may retain a different start time. The first stored sample is normally centred inside the first sample interval.\n"
 U"How do I use it? Select one Sound and read Info. Use this with Get end time or Get total duration when aligning a Sound with annotations.")
SOUND_HELP(U"Get end time", U"Report the Sound object's end boundary in seconds, which is different from the centre time of its last sample.",
 U"What does end time tell me? Where the object's time domain ends, including any time offset. It is not necessarily equal to the recording's duration.\n"
 U"How do I use it? Read Info for one Sound. Duration is end time minus start time; use Get total duration for that difference directly.")
SOUND_HELP(U"Get total duration", U"Report the Sound's time-domain length in seconds: end time minus start time.",
 U"Why is this different from end time? A Sound starting at 5 seconds and ending at 7 seconds has a duration of 2 seconds. Its timeline can be offset without changing its length.\n"
 U"How do I use it? Select one Sound and read Info. This reports the whole object's span, not the duration of the selection inside an editor.")
SOUND_HELP(U"Shift times by...", U"Add a time offset to the Sound's timeline without changing its sample values, duration, or playback pitch.",
 U"What shifts? The start, end and sample times all move by the same amount. This can align a recording with another object's timeline. It does not add silence at the beginning.\n"
 U"How do I use it? Enter the offset in seconds: positive moves later, negative earlier. Inspect start and end times afterward. Shift a related TextGrid appropriately as well if you need to preserve their alignment.")
SOUND_HELP(U"Shift times to...", U"Move the Sound's start, centre or end to a chosen time while preserving its duration and waveform.",
 U"What is the anchor? The point you select in the dialog: start, centre or end. Praat calculates a uniform time shift so that point lands at To time. No samples are inserted or removed.\n"
 U"How do I use it? Choose the anchor and destination in seconds, then check the new time boundaries and annotation alignment. This changes the existing object's timing, not its playback speed.")
SOUND_HELP(U"Scale times by...", U"Stretch or compress the existing Sound's time coordinates by a factor, changing duration and playback pitch without resampling its values.",
 U"What does time scaling do? Sample positions become farther apart or closer together. A factor above 1 makes playback longer and lowers frequencies; a factor below 1 makes it shorter and raises them. It is not pitch-preserving time stretching.\n"
 U"How do I use it? Copy the Sound if needed, enter a positive factor, then inspect timing and listen. For duration changes intended to preserve pitch, consider Lengthen (overlap-add) or a Manipulation object.")
SOUND_HELP(U"Scale times to...", U"Fit the existing Sound's timeline between new start and end times, changing duration and playback pitch when the span changes.",
 U"What happens to the samples? Their values stay the same but their time spacing changes. Changing the span changes playback rate; a uniform shift alone would leave duration and pitch unchanged.\n"
 U"How do I use it? Enter start and end times with the end after the start. Check the result and any related annotations. Use Shift times to when only an offset is intended, or pitch-preserving time manipulation when speed and pitch must be controlled separately.")
SOUND_HELP(U"Shift times to between zero and physical duration", U"Reset the Sound's time boundaries to zero and its sample-count-based physical duration, preserving sample values and sampling period.",
 U"Why use this command? It restores a zero-based time domain consistent with the stored sample count and interval. It can tidy the timing of an extracted or offset Sound.\n"
 U"How do I use it? Select the Sound and run the command, then check start, end and duration. It does not insert silence or stretch the samples. Check related annotations if they used the previous time coordinates.")
SOUND_HELP(U"Pre-emphasize (in-place)...", U"Boost higher frequencies relative to lower ones in the existing Sound, then scale its peak to 0.99.",
 U"What is pre-emphasis? A rising frequency weighting used in some speech analyses to make high-frequency detail more prominent. From frequency (Hz) controls its gradual onset; it is not a hard cutoff.\n"
 U"Why mention in-place? The selected Sound is changed, and this command also rescales its peak. Use Copy first to preserve the original, or Filter (pre-emphasis) for a new Sound. Listen and compare the spectrum; noise can become more prominent too.")
SOUND_HELP(U"De-emphasize (in-place)...", U"Reduce higher frequencies relative to lower ones in the existing Sound, then scale its peak to 0.99.",
 U"What is de-emphasis? A falling frequency weighting, used for example to counter pre-emphasis. From frequency in Hz controls where the reduction becomes important. It is not the same as deleting everything above a boundary.\n"
 U"How do I use it? Copy the Sound if needed, set From frequency and inspect or listen to the changed object. This version changes samples and rescales the peak; Filter (de-emphasis) creates a separate Sound instead.")

SOUND_HELP(U"To Formant (burg)...", U"Estimate vocal-tract resonance frequencies and bandwidths over time using Burg's method; create a Formant object for studying vowel-like spectral structure.",
 U"What are formants? Resonances shaped by the vocal tract, often labelled F1, F2 and so on from low to high frequency. Frequency locates a resonance in Hz; bandwidth describes its width. They differ from pitch, the repetition frequency of voiced speech.\n"
 U"What does Burg mean? The name of a linear-prediction estimation method, not a kind of vowel. Praat fits a simplified resonance model to short stretches of audio. The result estimates resonances; it does not directly measure vocal-tract shape.\n"
 U"How do I use it? Review the formant ceiling, maximum formant count, window length and time step. The ceiling limits the frequency region modelled, and the window determines how much audio contributes at once. Inspect the new Formant object's tracks against a spectrogram. Plausible-looking tracks can still be wrong, especially for high pitch, noise or mismatched settings.")
SOUND_HELP(U"To Formant (robust)...", U"Create a Formant object using an iterative method that reduces the influence of unusually large sample deviations when estimating resonances.",
 U"What is robust here? The algorithm repeatedly adjusts a linear-prediction fit with selective sample weighting. It aims to reduce some outlier effects; it does not guarantee correct formants or remove all noise. Formants are vocal-tract resonance estimates, measured in Hz, with bandwidths describing their widths.\n"
 U"How do I use it? Set the formant ceiling and analysis window as carefully as for Burg analysis, then review the standard-deviation threshold, iteration limit and tolerance. These control weighting and when refinement stops. Compare the new Formant tracks with the spectrogram and, if useful, Burg results. The source Sound remains unchanged.")
SOUND_HELP(U"To Formant (keep all)...", U"Create Burg-based Formant estimates while retaining near-edge resonances normally discarded; an experimental alternative rather than the usual vowel-analysis command.",
 U"What is kept? Estimates below 50 Hz or within 50 Hz of the formant ceiling are retained as well. These can reflect modelling artefacts rather than useful F1/F2 tracks.\n"
 U"Why use it? It can be useful for specialised resynthesis comparisons, where a complete model matters. The manual does not recommend it for general formant measurement.\n"
 U"How do I use it? Review the same analysis settings as To Formant (burg), create the Formant object and inspect the extra estimates. Use the standard Burg command as the ordinary starting point for vowel resonances.")
SOUND_HELP(U"To Formant (sl)...", U"Create Formant estimates with the experimental Split-Levinson method, which forces a requested count of resonances even when the signal does not support them.",
 U"What does sl mean? Split Levinson, the name of an alternative linear-prediction algorithm. Its output contains resonance frequencies, but this implementation assigns bandwidths an arbitrary 50 Hz value rather than estimating them.\n"
 U"Why the caveat? A full set of tracks can appear even when those formants are not present. The manual recommends Burg for ordinary formant work.\n"
 U"How do I use it? Use this for a deliberate method comparison, review the analysis settings and inspect the new Formant object against the spectrum. Do not interpret its bandwidths as measured resonance widths.")
SOUND_HELP(U"To FormantPath...", U"Create alternative formant analyses at several frequency ceilings in a FormantPath, so you can inspect and choose suitable tracks over time.",
 U"What is a FormantPath? A collection of alternative resonance analyses and a path choosing among them. A formant ceiling is the upper frequency used to fit a model; different ceilings can lead to different F1/F2 estimates.\n"
 U"Why use it? A single ceiling may not work equally well for all parts of a recording. This object supports comparing alternatives rather than assuming one setting is always right.\n"
 U"How do I use it? Set the middle ceiling, ceiling step size and steps up/down, then choose the LPC analysis model and time settings. LPC means linear predictive coding, a model using neighbouring samples to predict the waveform. Inspect the new FormantPath in its editor. The optional sources setting also publishes a multichannel Sound of analysis sources; the original Sound is retained.")
SOUND_HELP(U"To FormantPath (burg)...", U"Create a FormantPath containing Burg formant analyses at several ceilings, allowing alternative resonance tracks to be compared and selected.",
 U"Why more than one ceiling? The ceiling is the highest frequency considered by each resonance model. Changing it can change the estimated formants, the vocal-tract resonances used in vowel analysis. A FormantPath keeps alternatives together.\n"
 U"How do I use it? Set the middle ceiling, step size and number of steps above and below it, alongside the usual Burg window and formant-count settings. Open the new FormantPath to inspect or choose tracks. The results are estimates to check against the spectrogram; no waveform samples are changed.")

// The shared part explains LPC once; the first paragraph identifies the method.
#define SOUND_LPC_GUIDE(method) method U"\nWhat is LPC? Linear predictive coding models a sample as a weighted combination of earlier samples. The stored coefficients describe a changing prediction filter, useful for spectral modelling, filtering and resynthesis. They are not directly a list of vowel formants.\nHow do I use it? Review Prediction order (number of coefficients), Window length (audio per fit), Time step (spacing between fits) and Pre-emphasis (higher-frequency weighting). These commands model up to half the Sound's sampling frequency; resample appropriately if you deliberately need a lower modelling range. For routine vowel formants, To Formant (burg) is the simpler starting point. Inspect the new LPC object; the original Sound is unchanged."
SOUND_HELP(U"To LPC (autocorrelation)...", U"Create a linear predictive coding (LPC) model of the Sound using the autocorrelation method, for spectral modelling or synthesis.",
 SOUND_LPC_GUIDE(U"What does autocorrelation mean here? The method estimates a prediction filter from how waveform samples relate to delayed samples within each analysis window. This is filter modelling, not the To Pitch autocorrelation command."))
SOUND_HELP(U"To LPC (covariance)...", U"Create a linear predictive coding (LPC) model with the covariance method, an alternative way to fit a time-varying prediction filter.",
 SOUND_LPC_GUIDE(U"What is covariance here? A measure of how sample values vary together, used by this algorithm to fit the prediction coefficients. It is a different fitting criterion from autocorrelation, so the resulting filters can differ."))
SOUND_HELP(U"To LPC (burg)...", U"Create a linear predictive coding (LPC) model with Burg's algorithm; store prediction-filter coefficients rather than formant tracks.",
 SOUND_LPC_GUIDE(U"What does Burg mean? A method named after its developer that fits forward and backward prediction errors. Although To Formant (burg) also uses linear prediction, these two commands have different settings and outputs."))
SOUND_HELP(U"To LPC (marple)...", U"Create a linear predictive coding (LPC) model using Marple's method, with tolerance settings controlling when fitting stops.",
 SOUND_LPC_GUIDE(U"What are the tolerances? Stopping criteria based on prediction-error size and improvement as the model is fitted. Marple names the estimation method; the tolerance values are not a frequency range or a target sound level."))
SOUND_HELP(U"To LPC (robust)...", U"Create a linear predictive coding (LPC) model by iteratively reducing the influence of unusually large deviations during fitting.",
 SOUND_LPC_GUIDE(U"What does robust mean? Samples with large deviations can receive less weight during repeated refinement. The standard-deviation threshold controls that weighting; the iteration limit and tolerance control stopping. Robust does not mean noise-free or guaranteed accurate."))
#undef SOUND_LPC_GUIDE
SOUND_HELP(U"To Intensity...", U"Create an Intensity object measuring recorded signal level over time in dB, rather than reporting one value for the whole Sound.", nullptr)
SOUND_HELP(U"To Manipulation...", U"Create a Manipulation object containing the Sound plus estimated pulses, an editable pitch tier and duration controls for resynthesis.",
 U"What is a Manipulation object? Praat's workspace for changing speech timing and pitch, then generating or playing a resynthesized version. Pulses estimate repeating vocal-fold events; a pitch tier stores control points for the repetition frequency.\n"
 U"Why make one? You can experiment with intonation or duration without replacing the original Sound. Creating the object only prepares the analysis; it does not yet apply a pitch change.\n"
 U"How do I use it? Set Time step and plausible pitch floor/ceiling in Hz. Praat uses a mono, mean-centred copy for this analysis; the original Sound stays unchanged. Open the new Manipulation with View & Edit, check its analysis, edit controls and compare playback. Publish a resynthesized Sound when needed and save it separately. Pulse errors can create audible artefacts.")
SOUND_HELP(U"To KlattGrid (simple)...", U"Estimate a simple set of speech-synthesis controls from the Sound, stored as a KlattGrid for later editing and synthesis.",
 U"What is a KlattGrid? A set of time-varying controls for Praat's formant synthesizer, including pitch, level and resonance settings. A formant is a vocal-tract resonance; a grid stores parameters to generate sound rather than the original waveform.\n"
 U"Why simple? This command estimates selected controls from pitch, formant and intensity analyses. It is a starting model, not an exact reconstruction of all details or automatic phonetic annotation.\n"
 U"How do I use it? Review the separate pitch, formant and intensity analysis settings, create the KlattGrid, then inspect or edit its parameters and synthesize a Sound for comparison. Poor analysis settings affect the model. The original recording stays available.")

SOUND_HELP(U"Convert to mono", U"Create a one-channel Sound by combining the input channels; multichannel inputs are averaged rather than kept separately.",
 U"What is mono? One audio channel. Averaging channels can be useful for analyses expecting mono, but opposite-polarity signals can partly cancel. This is not speaker separation.\n"
 U"How do I use it? Select a Sound and inspect or play the new mono Sound. Use Extract one channel instead if you want only the left, right or another particular channel. The original is retained.")
SOUND_HELP(U"Convert to stereo", U"Create a two-channel Sound from the selected Sound; a mono input is duplicated into left and right channels.",
 U"What does duplication mean? Both output channels carry the same mono waveform. It does not create independent microphone positions or a new stereo spatial scene.\n"
 U"How do I use it? Select a mono Sound to duplicate it, or a stereo Sound to copy it. This command does not choose two channels from an input with more than two channels; use Extract channels for that. To place two different selected recordings in separate channels, use Combine to stereo instead.")
SOUND_HELP(U"Extract all channels", U"Create a separate mono Sound for every channel of the selected Sound, keeping the original multichannel object.",
 U"What is a channel? One stream of sample amplitudes, such as left or right. Extracting separates those streams without attempting to separate individual voices mixed within a channel.\n"
 U"How do I use it? Select one Sound and run the command. New channel Sounds appear in Objects with channel-based names. Play or analyse them individually, and save them separately if files are needed.")
SOUND_HELP(U"Extract one channel...", U"Copy one chosen audio channel into a new mono Sound, leaving the source Sound unchanged.",
 U"When is this useful? A stereo or multichannel recording may contain different microphones, speakers or signals in its channels. Choosing a channel preserves that stream instead of averaging it with the others.\n"
 U"How do I use it? Choose a channel number, with 1 normally left and 2 right for stereo, then inspect or play the new Sound. Channel identity depends on how the recording was made.")
SOUND_HELP(U"Extract channels...", U"Create one new Sound containing the selected channel numbers from each source Sound.",
 U"How is this different from Extract all channels? Several requested channels are kept together in one output Sound. Extract all channels produces separate mono objects. Channel numbers refer to existing streams, not detected speakers.\n"
 U"How do I use it? Enter the channel-number list or ranges and inspect the result's count and ordering. The selected source Sound is retained.")
SOUND_HELP(U"Extract part...", U"Copy a specified time interval into a new Sound, with options for edge tapering and preserving the original time coordinates.",
 U"What is extracted? A stretch of waveform samples, not an editor's current selection unless you enter matching times. The original Sound remains unchanged. A window shape weights the extracted samples and can taper the edges; rectangular keeps their amplitudes untapered.\n"
 U"How do I use it? Set From and To time in seconds, review Window shape and Relative width, and decide whether to Preserve times. Preserving times helps align with existing annotations; otherwise the new clip uses its own time origin. Inspect and play the new Sound.")
SOUND_HELP(U"Extract part for overlap...", U"Extract a Sound segment with extra material for a later overlapping join, rather than simply taking the exact requested interval.",
 U"Why include extra material? Cross-fading or overlap-add needs sound around a join so one segment can fade out while another fades in. Overlap duration controls this margin.\n"
 U"How do I use it? Enter the intended time range and overlap duration, then inspect the new Sound's boundaries before using it in a join. This extraction alone does not combine clips or change the source recording.")
SOUND_HELP(U"Resample...", U"Create a Sound at a new sampling frequency while preserving its duration and intended pitch, by calculating a new grid of sample values.",
 U"What is resampling? Reconstructing the waveform on a differently spaced time grid. Sampling frequency in Hz is samples per second, not the speaker's pitch. Lowering it also limits the highest frequencies that can be represented.\n"
 U"What is Precision? The interpolation depth in samples; interpolation estimates values between original positions. Higher precision changes the calculation's accuracy and cost.\n"
 U"How do I use it? Enter the desired sampling frequency and precision, then compare the new Sound with the original. Raising the rate does not recover frequencies missing from the original. Override sampling frequency instead reinterprets the same samples and changes speed/pitch.")
SOUND_HELP(U"Lengthen (overlap-add)...", U"Create a longer or shorter Sound using pitch-synchronous overlap-add, aiming to change duration while retaining pitch.",
 U"What is overlap-add? Praat takes short pieces guided by estimated voicing periods and overlaps them to produce a new timing pattern. Pitch-synchronous means using the repetition timing of voiced sound. PSOLA is another name used for this family of methods.\n"
 U"How do I use it? Set a plausible pitch floor and ceiling and a duration factor. A factor above 1 lengthens; below 1 shortens. Listen for artefacts, particularly where pulse detection is poor or speech is unvoiced. The new Sound retains the source for comparison; this differs from changing the declared sampling rate.")
SOUND_HELP(U"Deepen band modulation...", U"Create a Sound with stronger level fluctuations within frequency bands, emphasising selected rates of spectral change.",
 U"What is modulation here? Changes in a band's strength over time, such as moving vowel resonances, rather than the audio frequency inside the band. The slow and fast modulation settings are rates of these changes in Hz.\n"
 U"Why use it? Experiment with enhancing rapid spectral changes. This is a specialised signal transformation, not simply raising pitch or making the whole recording louder.\n"
 U"How do I use it? Review enhancement in dB, the audio-frequency range, the slow/fast modulation limits and band smoothing. Smoothing softens frequency boundaries. Inspect and listen to the new Sound, especially for unwanted effects on speech details.")
SOUND_HELP(U"Down to Matrix", U"Copy the Sound's stored sample grid into a Matrix: channels become rows and sample positions become columns.",
 U"What is a Matrix in Praat? A rectangular array of numbers with coordinate information. Here the numbers are waveform amplitudes, not a new spectrum or a correlation table.\n"
 U"Why use it? Access the data with general matrix operations or inspection. The result is not automatically an audio file. Select the new Matrix and inspect it; the original Sound remains unchanged.")
SOUND_HELP(U"Change gender...", U"Create a Sound with altered formant frequencies, pitch and duration using resynthesis. The command changes acoustic characteristics, not a person's gender.",
 U"What can be changed? Formants are vocal-tract resonance estimates; a formant shift ratio moves them. Pitch median sets the centre of the repetition-frequency range, pitch range factor changes variation around it, and duration factor changes length.\n"
 U"Why use it? Explore how these acoustic cues affect a voice or create experimental stimuli. The historic command name is not a claim that gender can be inferred or changed by one acoustic transformation.\n"
 U"How do I use it? Review pitch detection floor and ceiling before setting the transformation controls. Create and listen to the new Sound alongside the original; poor pitch detection or large transformations can produce artefacts.")

SOUND_HELP(U"To TextGrid...", nullptr, nullptr)
SOUND_HELP(U"To TextGrid (silences)...", U"Create a TextGrid marking relatively quiet and sounding intervals, using intensity thresholds; it does not recognise words or distinguish speech from every other sound.",
 U"What is a TextGrid? A separate timeline of labelled intervals and boundaries. This command fills it automatically using recorded signal strength. A silence threshold in dB is relative to the analysis level, not a guarantee of physical silence.\n"
 U"How do I use it? Review pitch floor and time step for the intensity calculation, the silence threshold, minimum silent/sounding durations and labels. Open the new TextGrid together with the Sound in View & Edit, listen and correct boundaries. Background noise and quiet speech affect the result; the Sound is unchanged.")
SOUND_HELP(U"To TextGrid (speech activity, Silero)...", U"Use the Silero voice-activity model to create a TextGrid labelling likely speech and non-speech intervals; it does not transcribe words.",
 U"What is Silero VAD? A trained voice-activity detection model. It estimates which stretches sound like speech; the result is an estimate that can be wrong for unfamiliar noises or quiet speech. The TextGrid stores timed labels separately from the Sound.\n"
 U"What do the settings mean? Probability threshold controls the speech decision, minimum gap controls which nearby stretches can merge, minimum segment excludes very short detections, and padding extends speech boundaries. Durations are in seconds.\n"
 U"How do I use it? Choose thresholds, duration rules and labels, then open the resulting TextGrid with the Sound. Listen around boundaries and correct mistakes before using the intervals as annotations.")
SOUND_HELP(U"To TextGrid (speech activity, LTSF)...", U"Detect speech-like intervals using long-term spectral flatness (LTSF) and optional intensity criteria; store the estimated boundaries in a TextGrid.",
 U"What is spectral flatness? How uniformly power is spread across frequency bands. A relatively flat spectrum can resemble noise, while speech often has more uneven and changing structure. LTSF measures this over time; it is not word recognition.\n"
 U"Why distinguish it from Silero? This method uses spectral and intensity rules instead of that trained voice-activity model. Its windows, frequency range and thresholds control what counts as speech-like sound.\n"
 U"How do I use it? Review short- and long-term windows, frequency range, flatness and intensity thresholds and minimum durations. Open the new TextGrid with the Sound and correct detections by listening. Non-speech sounds can also have uneven spectra, and some speech resembles noise.")
SOUND_HELP(U"Annotation tutorial", U"Open Praat's teaching material on creating and editing timed labels, normally stored in TextGrid objects.",
 U"What will I learn? Interval tiers label spans such as words, while point tiers mark individual moments. TextGrid annotations are stored separately from the Sound but can be edited alongside it.\n"
 U"How do I use it? Read the tutorial, then create a TextGrid from the Sound and open the two objects together with View & Edit. Opening this tutorial does not run detection or change the recording.")
SOUND_HELP(U"Filter (pass Hann band)...", nullptr, nullptr)
SOUND_HELP(U"Filter (stop Hann band)...", nullptr, nullptr)
SOUND_HELP(U"Filter (formula)...", U"Create a filtered Sound by applying your formula to its frequency-domain Spectrum, then transforming it back to audio.", nullptr)
SOUND_HELP(U"Filter (one formant)...", U"Create a Sound filtered through one resonance specified by its centre frequency and bandwidth, useful for modelling a formant-like effect.", nullptr)
SOUND_HELP(U"Filter (pre-emphasis)...", U"Create a Sound with higher frequencies boosted relative to lower ones, often used to prepare speech for spectral analysis.", nullptr)
SOUND_HELP(U"Filter (de-emphasis)...", U"Create a Sound with higher frequencies reduced relative to lower ones, for example to counter pre-emphasis.", nullptr)
SOUND_HELP(U"Filter with one formant (in-place)...", nullptr, nullptr)
SOUND_HELP(U"Filter (gammatone)...", U"Create a Sound filtered around a chosen frequency using a gammatone response, a model of auditory frequency filtering.", nullptr)
SOUND_HELP(U"Reduce noise...", U"Estimate background noise from a chosen noise-only interval and create a Sound with that noise reduced; wanted detail can also be affected.", nullptr)
SOUND_HELP(U"To Sound (derivative)...", U"Create a new waveform representing the rate of change of the Sound, with low-pass filtering to limit high-frequency amplification.", nullptr)
SOUND_HELP(U"Filtering tutorial", U"Open Praat's explanation of frequency filtering, including how filters shape a Sound and how to use them.",
 U"What is a filter? A process that changes the strength of frequency components. Hz measures cycles per second; a band is a range of those frequencies. Different filters keep, reduce or emphasise different regions.\n"
 U"How do I use it? Read the tutorial, then select the specific filter matching your aim. This button opens documentation and does not process the selected Sound.")
SOUND_HELP(U"Concatenate", nullptr,
 U"What does concatenate mean? Place Sounds one after another in time, creating one longer Sound. It differs from Combine to stereo, which places audio in separate simultaneous channels.\n"
 U"How do I use it? Select compatible Sounds and check their Objects list order; that determines the sequence. They need equal sampling frequencies and equal channel counts. Run the command and listen to the new Sound, especially at joins. The originals remain. Use overlap for a cross-fade or recoverably for a TextGrid identifying the pieces.")
SOUND_HELP(U"Concatenate recoverably", nullptr, nullptr)
SOUND_HELP(U"Concatenate with overlap...", nullptr, nullptr)
SOUND_HELP(U"Combine to stereo", nullptr,
 U"What does combine mean here? Stack the selected Sounds' channels into one multichannel Sound. Two mono inputs give two channels, conventionally left and right. Inputs with more channels can produce a result with more than two channels despite the command's name. They are not joined in time or mixed into one channel.\n"
 U"How do I use it? Select Sounds with the same sampling frequency and check their Objects list order, which determines channel order. Their time coordinates determine alignment; the output covers the combined time span, filling missing parts of a channel with zeros. Inspect and play the new Sound while keeping the originals for comparison.")
SOUND_HELP(U"Combine into SoundList", nullptr,
 U"What is a SoundList? A collection object containing separate Sounds, rather than one long waveform. The members are not mixed or concatenated.\n"
 U"Why use it? Manage related recordings as a collection. How do I use it? Select the Sounds, create the SoundList and inspect it or extract its Sounds with the collection's commands. Save any results you need as appropriate; making a collection is not exporting audio files.")
SOUND_HELP(U"Combine into SoundSet", nullptr,
 U"What is a SoundSet? A collection of Sound members that can be passed to commands expecting a SoundSet, for example specialised pattern-processing workflows. Creating it keeps member Sounds separate, rather than mixing them or joining them end to end.\n"
 U"How do I use it? Select the Sounds, create the SoundSet and inspect or extract its members. Use this object when your next command requires a set; choose Concatenate for one sequence of audio or Combine to stereo for separate channels.")
SOUND_HELP(U"Convolve...", U"Create a Sound by applying one selected waveform as a response to the other, as when applying a room impulse response to a recording.",
 U"What is convolution? Each input sample contributes a scaled, shifted copy of the other waveform; these copies add together. An impulse response describes how a system, such as a room, responds to a short impulse. Convolving with it models that response on another sound.\n"
 U"How do I use it? Select two compatible Sounds, choose amplitude scaling and the assumption for signal outside their time domains, then inspect the new Sound. Integral, sum, normalize and peak scaling serve different purposes and affect magnitudes. Duration and peaks can change. This is signal processing, not simple end-to-end joining; the originals remain.")
SOUND_HELP(U"Cross-correlate...", U"Create a Sound of similarity values over time lag by sliding one selected waveform against the other; useful for investigating delays.",
 U"What does the result represent? Cross-correlation compares the two waveforms at different offsets, called lags. A peak can indicate a delay with a strong match. Although stored as a Sound, the result's time axis is lag, not the original recording timeline.\n"
 U"How do I use it? Select two compatible Sounds and review amplitude scaling and signal-outside-domain settings. Draw or inspect the result for peaks; reversing input order reverses the lag direction. Similarity does not prove that the recordings contain the same words. The original Sounds are unchanged.")
SOUND_HELP(U"To ParamCurve", nullptr, nullptr)
SOUND_HELP(U"To DTW...", U"Create a dynamic time warping (DTW) alignment between two Sounds, mapping similar spectral patterns that may occur at different speeds.",
 U"What is dynamic time warping? A method for finding a path between time positions in two sequences. Here Praat compares features from successive short sound windows. A curved path describes changing timing relationships, not a newly stretched recording.\n"
 U"What do the constraints do? The Sakoe-Chiba band limits how far an alignment can depart from its allowed timing region. Slope limits restrict extreme local speed differences. Window length and time step determine the analysed features and their spacing.\n"
 U"How do I use it? Select two appropriate recordings, review these settings and inspect the new DTW object. Check plausible correspondences by listening. An alignment is a model of similarity, not automatic word transcription or proof of equivalent content.")
SOUND_HELP(U"To CrossCorrelationTable...", U"Create a table measuring relationships among channels of one Sound at a chosen time lag, useful for multichannel analysis.",
 U"What is cross-correlation? A measure of how two signals vary together when one is shifted by a delay, or lag. The table collects comparisons between channel pairs. At zero lag it describes their simultaneous relationships; the values are not automatically bounded correlation coefficients.\n"
 U"How do I use it? Choose the time range and lag in seconds and inspect the new CrossCorrelationTable. It is a table of channel statistics, not a Sound or a timeline of pitch. It can support source-separation methods; the original Sound is unchanged.")
SOUND_HELP(U"To CrossCorrelationTable (combined)...", U"Create a CrossCorrelationTable using channels from two selected Sounds together, for comparing their channel relationships at a chosen lag.",
 U"What is combined? Channels from both Sounds contribute to one statistical comparison. Cross-correlation measures how signals vary together with a time offset; it does not mix the recordings into a playable output.\n"
 U"How do I use it? Select two compatible Sounds, set the time range and lag in seconds, and inspect the new table. Its values describe channel relationships and scaling, not semantic similarity or a recognised transcript. The original Sounds are retained.")
SOUND_HELP(U"To Sound (bss)...", U"Attempt blind source separation (BSS) on a multichannel recording, creating a new Sound whose channels estimate underlying sources.",
 U"What does blind mean? The algorithm starts without knowing the original source signals or how they were mixed into the recorded channels. It uses statistical relationships between channels at several time lags to estimate an unmixing.\n"
 U"When is it useful? Recordings with multiple channels containing mixtures of sources that approximately satisfy the method's assumptions. It does not simply recognise speakers, and cannot guarantee separation of an arbitrary mono recording. Source order and scaling can be ambiguous.\n"
 U"How do I use it? Review the fitting time range, number of cross-correlations, lag step, iteration limit, tolerance and method. These control which statistics are fitted and when fitting stops. Listen to each output channel and compare with the source; the original remains available.")
SOUND_HELP(U"To Sound (white channels)...", U"Create a Sound with decorrelated, variance-normalised components across channels, a statistical preprocessing step called whitening.",
 U"What does white mean? Here whitening removes second-order channel correlations and normalises retained components using a statistical transformation. It is not adding white noise or making voices sound brighter. It does not by itself establish independent sources.\n"
 U"What is variance fraction? How much of the original signal variation to retain when choosing components. Reducing it can reduce the output dimensionality and discard weaker components.\n"
 U"How do I use it? Select a multichannel Sound, review the variance fraction, and inspect or listen to the new Sound's channels. Use it deliberately as preprocessing, for example before source analysis. Keep the original to assess what was changed.")

SOUND_HELP(U"Draw -", U"Prepare waveform figures in the Picture window, including ordinary drawing and conditional shading; these commands do not edit audio.",
 U"Which command should I choose? Draw plots amplitude against time. Draw where shows only portions satisfying a formula. Paint where fills relative to a chosen level, and Paint enclosed shades between two selected waveforms.\n"
 U"How do I use this in Praat? Choose the figure region in Picture, then select the Sound and a drawing command. Review time and amplitude limits. For an interactive waveform with playback, use View & Edit instead.")
SOUND_HELP(U"Query -", U"Report properties and measurements of one selected Sound in Info, including its time grid, waveform amplitudes and overall level.",
 U"What can I ask? Query time domain reports start, end and duration. Query time sampling reports sample count and spacing. Value and peak commands measure signed amplitudes, mean/RMS describe waveform statistics, and energy/power/intensity describe signal strength in different ways.\n"
 U"How do I interpret the result? Read the units: seconds for time, Hz for sampling frequency, amplitude units for samples, and dB for logarithmic level. Sampling frequency is not pitch, and amplitude is not perceived loudness. Select one Sound, choose a query, review any range and read Info. These commands do not change the samples.")
SOUND_HELP(U"Query time domain", U"Report the selected Sound's start boundary, end boundary or duration in seconds.",
 U"What is a time domain? The span of time covered by the object. Get start time and Get end time report its boundaries; Get total duration reports their difference.\n"
 U"Why distinguish these? A Sound can start at a nonzero time after extraction or alignment. Its end time then differs from its duration. These queries concern the whole object, not the highlighted region in a Sound editor.")
SOUND_HELP(U"Query time sampling", U"Inspect the Sound's evenly spaced sample grid: count, sampling frequency, period, and conversion between times and sample numbers.",
 U"What is time sampling? Recording a waveform as amplitude values at regular time intervals. Get number of samples counts positions per channel; sampling frequency is positions per second; sampling period is seconds between positions.\n"
 U"Which conversion should I use? Get time from sample number maps an index to seconds, while Get sample number from time can return a fractional index between samples. List all sample times prints the entire grid. None of these estimates the speaker's pitch or alters the recording.")
SOUND_HELP(U"Modify -", U"Change the selected Sound itself: edit timing, sample values, gain or in-place filtering. Make a Copy when the original must be kept.",
 U"Which change do I need? Modify times changes time coordinates; shifting moves the time origin, while scaling also changes duration and playback pitch. Reverse reverses audio order. Formula and Set commands replace samples. Add shifts their baseline; Subtract mean centres it.\n"
 U"How do gain commands differ? Multiply applies a factor, Scale peak targets the largest amplitude, and Scale intensity targets an averaged dB level. Multiply by window weights different times, usually tapering edges. In-place filters alter the same object.\n"
 U"How do I use it? Copy before an experiment, choose the specific change and inspect or listen afterward. Changes are in memory until saved/exported, and a previously saved source file is not automatically rewritten.")
SOUND_HELP(U"Modify times", U"Change the Sound's time coordinates: shift its position on a timeline or rescale its duration and playback rate.",
 U"What is a shift? Add an offset without changing the sample spacing, duration or pitch. Shift times by uses an offset; Shift times to anchors the start, centre or end at a destination.\n"
 U"What is scaling? Changing the spacing of existing samples, affecting duration and pitch. Scale times by uses a factor; Scale times to specifies new boundaries. For pitch-preserving duration changes, use specialised lengthening or Manipulation. Keep related annotations aligned deliberately.")
SOUND_HELP(U"In-place filters", U"Apply resonance, pre-emphasis or de-emphasis directly to the existing Sound instead of creating a separate filtered object.",
 U"What does in-place mean? The selected object's sample data is replaced. Filter with one formant emphasises one chosen resonance; pre-emphasis boosts higher frequencies relative to lower ones, and de-emphasis does the opposite. The emphasis commands also rescale the peak.\n"
 U"How do I use it? Copy the Sound first if you need the original. Choose a filter, review its settings, confirm and listen. The similarly named commands under Filter usually create new Sounds for comparison.")
SOUND_HELP(U"Annotate -", U"Create TextGrid timelines of labels and boundaries for the Sound, either empty for manual annotation or initially marked by detection methods.",
 U"What is a TextGrid? A separate annotation object with tiers: interval tiers label stretches, and point tiers label moments. It is aligned with audio by time, not stored as edits to the waveform.\n"
 U"Which option fits my task? To TextGrid creates empty named tiers. Silero estimates speech using a trained voice-activity model. LTSF uses spectral flatness, a measure of how evenly power is distributed. Silences uses intensity thresholds. None of these automatically writes a transcript.\n"
 U"How do I use it? Create the appropriate TextGrid, select it together with the Sound, then View & Edit to listen and adjust labels or boundaries. Save the TextGrid separately. Use Annotation tutorial for a guided introduction.")
SOUND_HELP(U"Analyse periodicity -", U"Analyse repetition in the Sound: estimate pitch, locate pulse times, measure harmonicity or inspect autocorrelation.",
 U"What is periodicity? A pattern repeating over time. Voiced speech often repeats because the vocal folds vibrate. The repeat rate is fundamental frequency (F0), in Hz; individual pulse times and regularity describe other aspects of that pattern.\n"
 U"Which result do I need? To Pitch creates frequency estimates over time. To PointProcess stores event times: choose periodic variants for pitch-guided pulse estimates, extrema for waveform peaks, or zeroes for sign crossings. Harmonicity (ac/cc) creates an HNR curve, comparing periodic and noise components; GNE creates a Matrix using another excitation measure. Autocorrelate creates a lag-based similarity signal.\n"
 U"How do I use it? Read the method's note, set appropriate frequency bounds and analysis windows, and inspect the new object with the original Sound. The pitch-method tutorial explains the alternatives. None of these replaces listening or guarantees correct estimates for noise and silence.")
SOUND_HELP(U"Analyse spectrum -", U"Analyse frequency content: choose an overall spectrum, a long-term average, time-frequency maps, resonance tracks or compact spectral features.",
 U"What is frequency content? A sound can be described as a mixture of slower and faster vibrations. Hz counts vibrations per second. Spectral analysis describes how these components contribute; it does not recognise words.\n"
 U"Which overview do I need? Spectrum stores overall frequency strengths and phases. Ltas (long-term average spectrum) summarises power in frequency bands across the recording. Spectrogram shows frequency energy changing over time. Cochleagram, BarkSpectrogram and MelSpectrogram use hearing-inspired frequency representations.\n"
 U"Which specialised analysis? Formant estimates vocal-tract resonances; FormantPath keeps alternative resonance fits. LPC stores a linear-prediction filter model. MFCCs are mel-frequency cepstral coefficients describing spectral shape. PowerCepstrogram shows cepstral repetition patterns; CPP measures cepstral peak prominence over time.\n"
 U"How do I use it? Choose according to the question, review the specific settings and inspect the resulting object. Extract a Sound segment first if the command otherwise analyses more audio than intended. The analysis creates objects without replacing the original Sound.")
SOUND_HELP(U"To Formant (hack)", U"Open experimental formant-analysis alternatives: keep otherwise discarded estimates or use Split Levinson. They are not the usual starting point for vowel measurements.",
 U"Why experimental? Keep all retains near-edge resonance estimates usually discarded by Burg analysis. Split Levinson forces a resonance count and assigns placeholder bandwidths, which can look plausible without representing real formants.\n"
 U"How do I choose? Use To Formant (burg) for ordinary work unless you have a specific reason to compare these methods. Read each submenu note before interpreting its Formant output.")
SOUND_HELP(U"To LPC", U"Choose a linear predictive coding (LPC) method to create a time-varying prediction-filter model of the Sound.",
 U"What is linear prediction? Modelling a waveform value as a weighted sum of earlier values. The coefficients describe a filter related to spectral shape; they are not directly a Pitch or Formant track.\n"
 U"What do the alternatives mean? Autocorrelation and covariance fit sample relationships differently. Burg uses forward and backward prediction. Marple adds tolerance-controlled estimation. Robust repeatedly adjusts sample weighting to reduce some outlier effects. All need an appropriate prediction order and analysis window.\n"
 U"How do I use it? Choose a method for a specific modelling purpose and read its settings. These commands use half the Sound's sampling frequency as their upper modelling limit. To Formant (burg), with an explicit formant ceiling, is the simpler route for ordinary vowel resonance analysis.")
SOUND_HELP(U"Manipulate -", U"Prepare an editable Manipulation object for changing pitch and duration and resynthesizing a version of the Sound.",
 U"How is this different from analysis? To Manipulation estimates pulse and pitch information and stores controls that can be edited before generating audio. Creating it does not immediately retune the Sound.\n"
 U"How do I use it? Choose To Manipulation, review pitch bounds, then View & Edit the resulting object. Inspect the pulse analysis, alter pitch or duration controls and compare playback before publishing a resynthesized Sound.")
SOUND_HELP(U"Convert -", U"Create related Sounds or data objects: change channels, extract clips, resample, transform voice characteristics or prepare multichannel statistics.",
 U"Which channel operation? Convert to mono averages channels, Convert to stereo duplicates mono into two channels, and Extract commands copy selected channels or time spans. They do not separate speakers mixed in a single channel.\n"
 U"Which timing operation? Resample changes the sample grid while preserving duration and intended pitch. Lengthen (overlap-add) aims to change duration without changing pitch. Deepen band modulation enhances changing band levels, and Change gender offers acoustic pitch/formant/duration transformations.\n"
 U"Which specialised outputs? CrossCorrelationTable stores channel relationships. BSS attempts blind source separation from multichannel mixtures. White channels performs statistical whitening. Down to Matrix copies the sample array for general numerical operations. Read the individual command's note before choosing it; a conversion may create a Sound or a different object type.")
SOUND_HELP(U"Filter -", U"Create processed versions of the Sound by keeping, reducing or emphasising frequency regions, or applying noise reduction and derivative processing.",
 U"What is a filter? It changes the relative strengths of frequency components. A band is a frequency range in Hz. Hann pass keeps a band; Hann stop reduces it; their smooth edge transitions reduce ringing from abrupt frequency changes.\n"
 U"Which other filter? Formula edits the frequency-domain Spectrum with an expression. One formant emphasises a specified resonance. Pre-emphasis raises higher frequencies relatively; de-emphasis lowers them. Gammatone models an auditory filter around a centre frequency.\n"
 U"Which other processing? Reduce noise estimates unwanted sound from a noise interval; it can also damage wanted detail. To Sound (derivative) emphasises the rate of waveform change and limits high-frequency amplification. These commands create new Sounds; the in-place variants in Modify change the original instead. Listen to the result alongside the original.")
SOUND_HELP(U"Combine -", U"Use selected Sounds together: arrange channels, join clips in time, collect recordings, or calculate relationships between signals.",
 U"How do the main choices differ? Combine to stereo places inputs in separate channels. Concatenate joins them end to end in Objects order; recoverably also creates a TextGrid identifying the pieces, while overlap cross-fades their ends. SoundList and SoundSet collect separate recordings without mixing them.\n"
 U"What about mathematical combinations? Convolve applies one waveform as a response to another. Cross-correlate compares waveforms over delay. A combined CrossCorrelationTable stores channel statistics. DTW (dynamic time warping) aligns changing spectral patterns at different speeds. ParamCurve plots one signal's values against another's.\n"
 U"How do I use it? Select the required compatible Sounds and check list order, sample rates and channels. Read the individual command's output and settings before running it. Combining does not always produce playable audio, and the originals are retained.")

SOUND_HELP(U"To Ltas...",
 U"Create a long-term average spectrum (Ltas): a summary of how the recording's power is distributed across frequencies over its whole duration.",
 U"What is an Ltas? Ltas means long-term average spectrum. It combines the whole selected Sound into one frequency profile, rather than showing when each sound occurred. Low frequencies are slower vibrations, high frequencies faster ones; hertz (Hz) means vibrations per second.\n"
 U"What does Praat produce? A new Ltas object in Objects. Its frequency bins are small neighbouring frequency ranges, with average spectral power density shown in decibels per hertz (dB/Hz). This describes the recorded signal; physical sound-pressure levels require a calibrated recording. The Sound is unchanged.\n"
 U"Why use it? Compare the overall spectral balance of recordings, such as how much energy lies in low versus high frequencies. Pauses, background noise, recording level and the chosen speech material all affect the result. It cannot show the timing of individual words.\n"
 U"How do I use it? Select a Sound, choose Bandwidth (Hz), and confirm. Bandwidth sets the width of each frequency bin: smaller bins retain finer frequency detail, while larger bins give a broader summary. Select the new Ltas and draw or query it. Extract part of the Sound first to analyse only that part. Use a Spectrogram instead when timing matters.")
SOUND_HELP(U"To Ltas (pitch-corrected)...",
 U"Create a long-term average spectrum of voiced speech while reducing the influence of fundamental frequency on the spectral profile. This analyses the voice; it does not retune the recording.",
 U"What does pitch-corrected mean here? Ltas means long-term average spectrum. In voiced speech, vocal-fold repetition creates harmonics: frequency components at multiples of the fundamental frequency (F0). This method tries to reduce F0's influence on the average spectral envelope, the broad shape across those components.\n"
 U"Why use this version? To study the broad spectral shape of the voiced parts, including vocal-tract resonances, with less influence from harmonic spacing. It is different from averaging all sound and silence with ordinary To Ltas. It does not correct a singer's tuning or change the Sound.\n"
 U"How do I use it? Select speech with usable voicing, set a realistic pitch floor and ceiling in Hz, and review the frequency limit, bin bandwidth and allowed pulse periods. A period is the time for one repetition. Inspect the new Ltas and compare suitable recordings; unreliable pulse detection can make the estimate unreliable.")
SOUND_HELP(U"To Spectrum...",
 U"Create a Spectrum describing the frequency components of the entire Sound, including their strengths and phases. It gives one overall frequency analysis, not a timeline.",
 U"What is a spectrum? It represents a sound as a combination of slow and fast vibrations. Frequency is measured in hertz (Hz), cycles per second. A Spectrum stores complex values: information about both the size and relative timing, or phase, of each frequency component.\n"
 U"Why use it in Praat? Inspect harmonic peaks, investigate filtering, or convert a modified Spectrum back to a Sound. Harmonics are components spaced at multiples of a repeating sound's fundamental frequency. Use To Spectrogram for a frequency picture that changes over time; use To Ltas for average power in frequency bands.\n"
 U"How do I use it? Select a Sound and review Fast. The fast Fourier transform (FFT) is a computational method; the fast option can add zero-valued samples internally to make its calculation efficient. Praat creates a separate Spectrum. Extract the desired Sound segment first if you want to analyse just one vowel or event.")
SOUND_HELP(U"To Spectrum (resampled)...",
 U"Create a Spectrum using a resampling-based calculation of the whole Sound's frequency components. Precision controls that numerical approximation.",
 U"What is being resampled? This specialised spectrum calculation uses interpolation and resampling internally. Interpolation estimates values between stored samples. The result is a Spectrum, which describes strengths and phases at frequencies in Hz, not a newly saved recording.\n"
 U"When would I use it? When you specifically need this alternative spectrum calculation. Ordinary To Spectrum is the simpler starting point for inspecting a recording's frequencies.\n"
 U"How do I use it? Select a Sound, review Precision (samples), and create the Spectrum. Precision controls the interpolation calculation. Compare with ordinary To Spectrum when checking numerical differences; the original Sound remains unchanged.")
SOUND_HELP(U"To Spectrogram...", U"Create a Spectrogram object showing how energy at different frequencies changes over time, for inspecting or drawing the Sound's spectral detail.", nullptr)
SOUND_HELP(U"To Spectrogram (pitch-dependent)...",
 U"Create a Spectrogram using frequency filters whose widths follow the estimated pitch. It shows spectral energy over time with pitch-dependent frequency smoothing.",
 U"What is different about this spectrogram? A spectrogram describes frequency energy at successive times. This version uses a bank of frequency filters with bandwidths related to the estimated pitch; bandwidth is how wide a frequency region each filter covers.\n"
 U"Why use it? It provides a pitch-dependent view of the spectral shape. The pitch estimate affects the filters, so an incorrect pitch can affect the result. It is not the standard fixed-window spectrogram and does not change the sound's pitch.\n"
 U"How do I use it? Set the analysis window and time step, filter frequencies and relative bandwidth, plus a pitch floor and ceiling appropriate for the recording. Praat creates a new Spectrogram. Compare it with the standard spectrogram and listen to the Sound.")
SOUND_HELP(U"To Cochleagram...",
 U"Create a Cochleagram: a time-varying model of how frequency regions in the inner ear respond to the Sound.",
 U"What is a cochleagram? The cochlea is the part of the inner ear that separates sound into frequency regions. Praat's Cochleagram models an excitation pattern across those regions over time; it is an analysis object, not a measurement taken inside a listener's ear.\n"
 U"What are Bark and forward masking? Bark is a frequency scale based on auditory frequency regions. Frequency resolution in Bark controls spacing on that scale. Forward masking models how an earlier sound influences the model's response shortly afterward.\n"
 U"How do I use it? Set Time step for spacing between analyses, Window length for how much audio each analysis uses, and review frequency resolution and forward-masking time. Inspect or paint the new Cochleagram. Use To Spectrogram for a more direct physical frequency analysis.")
SOUND_HELP(U"To BarkSpectrogram...",
 U"Create a time-frequency power analysis using Bark-spaced auditory filters, stored as a BarkSpectrogram.",
 U"What is Bark? It is a frequency scale based on how the ear groups nearby frequencies. A filter bank measures energy through a series of overlapping frequency bands. Bark spacing becomes wider in Hz at higher frequencies.\n"
 U"Why use it? Study the sound through an auditory frequency representation rather than equally spaced frequency bins. It is not a transcript or a full simulation of hearing.\n"
 U"How do I use it? Set the analysis window and time step, then the first filter, spacing and maximum frequency on the Bark scale. Praat creates a BarkSpectrogram; inspect or paint it. Window length trades timing detail against frequency detail.")
SOUND_HELP(U"To MelSpectrogram...",
 U"Create a time-frequency power analysis using mel-spaced filters, stored as a MelSpectrogram and suitable for calculating MFCC features.",
 U"What is mel? It is a perceptually motivated frequency scale: equal changes in mel do not mean equal changes in Hz. Overlapping filters collect energy from neighbouring frequency ranges.\n"
 U"Why use it in Praat? It summarises speech spectra on a scale often used in speech processing. It can be converted into mel-frequency cepstral coefficients (MFCCs), compact numerical descriptions of spectral shape.\n"
 U"How do I use it? Set the analysis window, time step, and mel filter spacing and range. Inspect the new MelSpectrogram. These are measurements across time, not audio channels or recognised words.")
SOUND_HELP(U"To PowerCepstrogram...",
 U"Create a PowerCepstrogram showing how strongly the Sound's spectrum has regularly spaced components at each time, useful for analysing voice periodicity.",
 U"What is a cepstrum? It analyses the shape and repeating spacing in a spectrum. A voiced sound often has regularly spaced harmonics, frequency components caused by repeated vocal-fold vibration. A cepstral peak can reflect that repetition.\n"
 U"What is quefrency? The horizontal coordinate of a single cepstrum is a time-like delay measured in seconds, called quefrency. A PowerCepstrogram stores successive power cepstra across recording time. Quefrency is not the moment in the recording.\n"
 U"Why use it? Inspect cepstral patterns or calculate cepstral peak prominence (CPP), a measure of how much a peak rises above a fitted background trend. It is not itself a diagnosis of voice health.\n"
 U"How do I use it? Review pitch floor, time step, maximum frequency and pre-emphasis. Pre-emphasis boosts higher frequencies before analysis. Create and inspect the new PowerCepstrogram; query its CPP-related measures as needed. The Sound is unchanged.")
SOUND_HELP(U"To CPP...",
 U"Create a CPP object tracking cepstral peak prominence over time: how strongly a harmonic repetition peak stands out from the cepstral background.",
 U"What does CPP stand for? Cepstral peak prominence. Praat analyses patterns in the spectrum: voiced speech often has harmonics, frequency components spaced at multiples of the vocal-fold repetition frequency. CPP measures a corresponding cepstral peak above a fitted trend, in dB.\n"
 U"What is quefrency? It is the delay coordinate of the cepstrum, measured in seconds. Pitch range limits which repetition periods are searched. Time and quefrency smoothing average neighbouring values before peak measurement.\n"
 U"How do I use it? Set a plausible pitch range and review analysis, smoothing, peak interpolation and trend-fitting settings. Interpolation estimates a peak between sampled positions. Praat creates a CPP object across time, not just one number. Inspect its curve or query it. Settings and recording quality affect comparisons; this value alone does not establish a voice disorder.")
SOUND_HELP(U"To MFCC...",
 U"Create mel-frequency cepstral coefficients (MFCCs): compact numerical features describing the Sound's changing spectral shape.",
 U"What do the terms mean? Mel is a perceptually motivated frequency scale. Cepstral coefficients are numbers derived from the pattern of energy across frequency bands. Praat first makes a mel-filtered spectral analysis, then transforms it into a set of coefficients at successive times.\n"
 U"Why use them? Compare or model speech sounds using a smaller set of features than a full spectrogram. MFCCs are not pitch values, formant tracks, or recognised text. A coefficient number is not a frequency in Hz.\n"
 U"How do I use it? Select a Sound, review the number of coefficients, window length, time step and filter-bank settings, and create the MFCC object. Use the same settings when comparing recordings. Inspect the coefficients or use them in a later comparison or modelling step.")

SOUND_HELP(U"To Pitch (filtered autocorrelation)...",
 U"Estimate vocal-fold repetition frequency over time using filtered autocorrelation; create a Pitch object for studying speech intonation.",
 U"What is pitch here? Pitch is perceived highness or lowness. Praat estimates fundamental frequency (F0), the repetition rate of voiced sound in hertz (Hz), cycles per second. Unvoiced consonants and silence need not have a meaningful F0.\n"
 U"What is filtered autocorrelation? Autocorrelation compares the waveform with delayed copies of itself to find repeating periods. This version filters the signal before making that comparison. Praat's pitch tutorial identifies it as the usual starting point for intonation analysis.\n"
 U"How do I use it? Select a Sound, set a plausible Pitch floor and Pitch top, and review the tracking settings. These bounds guide candidate selection; they are not volume controls. Inspect the new Pitch object and compare with listening and the Sound editor. Large jumps can be octave errors, where the estimate is doubled or halved. The Sound is unchanged.")
SOUND_HELP(U"To Pitch (raw autocorrelation)...",
 U"Estimate repeating periods directly from the unfiltered Sound using autocorrelation and create a Pitch object.",
 U"What does raw autocorrelation mean? Praat compares delayed versions of the waveform to find repetition without the preliminary filtering of filtered autocorrelation. The estimated fundamental frequency (F0) is measured in Hz, cycles per second.\n"
 U"Why this alternative? It is an established pitch-analysis method and is also used in some pulse-analysis workflows. Results can differ from the filtered method; the name does not mean it is automatically more accurate.\n"
 U"How do I use it? Set Pitch floor and ceiling to cover the expected voicing range and inspect the new Pitch object for missed voicing or doubled/halved estimates. For ordinary intonation, compare with filtered autocorrelation. No audio samples are changed.")
SOUND_HELP(U"To Pitch (raw cross-correlation)...",
 U"Create a Pitch object by comparing nearby waveform segments directly, a method used in Praat voice-analysis workflows.",
 U"What is cross-correlation? It measures how closely two waveform segments match as one is shifted in time. Repeated matches help Praat estimate the period between vocal-fold cycles, then its inverse: fundamental frequency in Hz. Raw means no preliminary low-pass filtering of the kind used by the filtered method.\n"
 U"Why choose it? Praat's teaching material distinguishes voice-analysis work from ordinary intonation tracking, for which filtered autocorrelation is the usual starting point. The methods can disagree on noise, voicing and octave choices.\n"
 U"How do I use it? Review the time step, pitch floor and ceiling and tracking thresholds. Create a Pitch object, inspect it and compare with the Sound and listening. This is an estimate of frequency, not an extraction of every glottal pulse.")
SOUND_HELP(U"To Pitch (filtered cross-correlation)...",
 U"Create a Pitch object using cross-correlation after filtering the Sound for analysis; estimate repetition frequency without changing the recording.",
 U"What does the method do? Cross-correlation compares waveform segments at different delays. Filtering changes which frequency components contribute to the analysis before Praat chooses a repeating period. Fundamental frequency (F0) is the repetition rate, measured in Hz.\n"
 U"Why use it? To compare this combination of filtering and period detection with the other pitch methods. Filtering can change which candidate is strongest; it does not guarantee the correct pitch.\n"
 U"How do I use it? Set pitch bounds and review filtering and tracking settings, then inspect the new Pitch object. Check voiced and unvoiced regions and possible doubled or halved estimates against the original Sound. Choose the method for your analysis purpose, using Praat's pitch-method tutorial.")
SOUND_HELP(U"To Pitch (shs)...",
 U"Create a Pitch object using subharmonic summation (SHS), which looks for frequency components that could belong to one harmonic series.",
 U"What is subharmonic summation? Voiced sound often contains components at multiples of a fundamental frequency. SHS combines compressed versions of the spectrum so related components support the same candidate fundamental. It estimates pitch from spectral structure rather than directly matching waveform cycles.\n"
 U"Why use it? It is an alternative pitch estimator for comparing spectral and waveform-based methods. A strong spectral pattern can still lead to an incorrect octave or voicing decision.\n"
 U"How do I use it? Set the pitch bounds, maximum analysed frequency, number of subharmonics and frequency-grid density. Subharmonics control how many related components can contribute. Inspect the new Pitch object and compare its estimates with listening and another pitch method.")
SOUND_HELP(U"To Pitch (SPINET)...",
 U"Create a Pitch object with SPINET, a spectral-integration model that combines evidence from a bank of hearing-inspired frequency filters.",
 U"What is SPINET? It is a spectral-integration pitch model. Praat analyses the Sound through many overlapping gammatone filters, shaped to resemble auditory frequency filtering, and combines their evidence for candidate repetition frequencies.\n"
 U"Why use it? Explore a hearing-inspired alternative to the usual pitch estimators. This model produces estimated pitch candidates; it does not reproduce everything a listener hears or recognise words.\n"
 U"How do I use it? Review the time step and window, minimum and maximum filter frequencies, number of filters, pitch ceiling and candidate count. Inspect the resulting Pitch object. More filters change the model's frequency sampling and cost; compare the result with the original Sound.")
SOUND_HELP(U"How to choose a pitch analysis method",
 U"Open Praat's tutorial comparing pitch-analysis methods and when to use them for intonation or voice analysis.",
 U"Why read this first? Several commands estimate the same general property with different methods. Pitch floor, upper limits, filtering and tracking choices affect the result.\n"
 U"What happens when I click? Praat opens its teaching page; it does not analyse or modify the selected Sound. Use the tutorial to choose a method, then return to the Sound commands.")
SOUND_HELP(U"To PointProcess (periodic, cc)...",
 U"Estimate vocal-fold pulse times using pitch analysis and waveform cross-correlation; create a PointProcess containing event times.",
 U"What is a PointProcess? A list of moments, in seconds. Here the moments estimate successive pulses in voiced sound. It is not a Pitch curve: Pitch gives a frequency over time, while pulse spacing can support cycle-to-cycle measurements.\n"
 U"What does cc mean? Cross-correlation, comparing nearby waveform cycles to locate repeating events. Praat first estimates pitch using raw autocorrelation, then uses that guidance to find pulses. These are acoustic estimates, not direct observations of the vocal folds.\n"
 U"How do I use it? Set a suitable pitch floor and ceiling, create the PointProcess and inspect pulse placement together with the Sound. Missing or extra pulses affect jitter and other voice measurements, so check the voiced region before using those measurements.")
SOUND_HELP(U"To PointProcess (periodic, peaks)...",
 U"Create pulse-time estimates by using pitch guidance to select waveform peaks, stored as a PointProcess.",
 U"What is being detected? Praat first estimates pitch with raw autocorrelation, then looks for waveform peaks consistent with those periods. The PointProcess stores times in seconds, not audio samples or pitch values.\n"
 U"Why distinguish it from extrema? Plain extrema detection marks local peaks without this pitch guidance. A waveform can have several local peaks during one vocal-fold cycle. The periodic method tries to select cycle-related events.\n"
 U"How do I use it? Review pitch floor and ceiling and whether maxima, minima or both should be included. Inspect the resulting points with the Sound, especially near voicing boundaries. Check pulse quality before measuring timing irregularity.")
SOUND_HELP(U"To PointProcess (extrema)...",
 U"Mark local waveform maxima and/or minima as event times in a new PointProcess; these are not necessarily vocal-fold pulses.",
 U"What are extrema? Peaks and troughs of the waveform: values greater or smaller than their neighbours. Multiple extrema can occur within one voiced cycle or in noise.\n"
 U"What is interpolation? An estimate between stored samples, used here to locate a peak more precisely. The PointProcess stores only event times.\n"
 U"How do I use it? Choose the Sound channel, whether to include maxima and/or minima, and interpolation. Inspect the new PointProcess with the waveform. Use a periodic pulse command instead if your purpose requires pitch-guided vocal-fold pulse estimates.")
SOUND_HELP(U"To PointProcess (zeroes)...",
 U"Mark times when a Sound channel crosses zero, creating a PointProcess of rising and/or falling crossings.",
 U"What is a zero crossing? A moment when waveform amplitude changes sign. A rising crossing goes from negative to positive; a falling crossing goes the other way. It is not necessarily a word boundary or one vocal-fold pulse.\n"
 U"Why use it? Study waveform crossings or create event times for a specialised analysis. Noise can add many crossings.\n"
 U"How do I use it? Select the channel and crossing directions, then inspect the new PointProcess against the Sound. For just the crossing nearest a given time, use Get nearest zero crossing instead.")
SOUND_HELP(U"To Harmonicity (cc)...",
 U"Create a Harmonicity object measuring harmonics-to-noise ratio (HNR) over time with cross-correlation: how strongly periodic sound stands out from noise.",
 U"What is harmonicity? A measure of the balance between repeating and non-repeating components. HNR means harmonics-to-noise ratio. Praat expresses it in dB; higher values indicate a stronger periodic component under this model. It is not a pitch value or a diagnosis.\n"
 U"Why cc? Cross-correlation compares waveform segments at different delays. Praat's manual describes this method as providing better time resolution than the autocorrelation version.\n"
 U"How do I use it? Review time step, pitch floor, silence threshold and periods per window. The floor and period count determine how much audio is analysed at a time. Inspect the new Harmonicity object, and compare equivalent voiced regions recorded under similar conditions.")
SOUND_HELP(U"To Harmonicity (ac)...",
 U"Create a Harmonicity object measuring harmonics-to-noise ratio (HNR) over time using autocorrelation.",
 U"What is HNR? The estimated strength of a repeating, harmonic component relative to a noise component, in dB. Autocorrelation compares the waveform with delayed versions of itself. A higher value means greater periodicity under this analysis, not higher pitch.\n"
 U"Why this version? It offers an alternative to the cross-correlation method. The manual notes a tradeoff between measurement sensitivity and time resolution; To Harmonicity (cc) is normally the starting point.\n"
 U"How do I use it? Set time step, pitch floor, silence threshold and periods per window. Longer windows include more cycles but blur changes over time. Inspect the new Harmonicity object and avoid treating silence or mixed voiced/unvoiced material as one comparable voice measurement.")
SOUND_HELP(U"To Harmonicity (gne)...",
 U"Calculate glottal-to-noise excitation (GNE) using relationships between frequency bands. Despite the menu name, this command creates a Matrix, not a Harmonicity curve.",
 U"What is GNE? Glottal-to-noise excitation compares slow amplitude patterns in different frequency bands to assess common excitation associated with voiced sound. A frequency band is a range in Hz. This is a different analysis from the harmonics-to-noise ratio (HNR) commands.\n"
 U"What does the result contain? A Matrix of band-comparison values. Its rows and columns do not form the usual Harmonicity time curve. Do not interpret it as HNR in dB or as a Pitch object.\n"
 U"How do I use it? Review the minimum and maximum frequencies, bandwidth and frequency step, run the command, and inspect the Matrix. Use the GNE method's interpretation for subsequent measurements; the output alone does not diagnose a voice condition.")
SOUND_HELP(U"Autocorrelate...",
 U"Create an autocorrelation Sound showing how well the waveform matches delayed copies of itself, useful for examining repetition intervals.",
 U"What is autocorrelation? Compare a signal with itself after shifting it by a delay, or lag. Strong matches at repeated lags can reveal a repeating period. Lag is measured in seconds; it is not the original recording timeline.\n"
 U"Why use it? Inspect repetition directly or study signal similarity. This command creates correlation values in a new Sound object; it does not automatically select a fundamental frequency as To Pitch does.\n"
 U"How do I use it? Review amplitude scaling and treatment of samples outside the Sound. Draw or inspect the result and read its horizontal axis as lag. Different scaling choices change correlation magnitudes, so use consistent settings when comparing results.")
