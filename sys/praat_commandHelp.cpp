#include "praatP.h"

// Exact command names and source class keep similarly named analyses distinct.
// nullptr preserves an existing explanation that was reviewed as sufficient.
struct SoundCommandHelp { conststring32 title, summary, guide; };
#define SOUND_HELP(title, summary, guide) { title, summary, guide },
static const SoundCommandHelp soundCommandHelp [] = {
#include "praat_SoundCommandHelp.inc"
};
#undef SOUND_HELP

static const SoundCommandHelp * soundHelp (Praat_Command action) {
	if (! action -> class1 || action -> class2 || ! str32equ (action -> class1 -> className, U"Sound"))
		return nullptr;
	for (const auto &help : soundCommandHelp)
		if (str32equ (action -> title.get(), help.title)) return & help;
	return nullptr;
}

static bool actionIsForClass (Praat_Command action, conststring32 className) {
	return action -> class1 && str32equ (action -> class1 -> className, className);
}

static bool actionIsStandardAnalysis (Praat_Command action, conststring32 title, conststring32 name) {
	return action -> callback && actionIsForClass (action, U"Sound") &&
		( str32equ (title, Melder_cat (name, U"...")) || Melder_startsWith (title, Melder_cat (name, U" (")) );
}

conststring32 praat_actionSummary (Praat_Command action) {
	if (const auto *help = soundHelp (action); help && help -> summary) return help -> summary;
	conststring32 title = action -> title.get();
	if (str32equ (title, U"View & Edit"))
		return U"Open Praat's editor for the selected object. For a Sound, it shows the waveform and listening and analysis controls; for a TextGrid, it shows annotation tiers.";
	if (str32equ (title, U"Play") && actionIsForClass (action, U"Sound"))
		return U"Play the selected Sounds through your audio output, in their Objects list order.";
	if (str32equ (title, U"Play"))
		return U"Hear Praat's acoustic representation of the selected object; what is played depends on its object type.";
	if (actionIsStandardAnalysis (action, title, U"To Pitch"))
		return U"Estimate how high or low the voice sounds over time. Creates a Pitch object that you can inspect or edit.";
	if (actionIsStandardAnalysis (action, title, U"To Formant"))
		return U"Estimate the resonant frequencies of the sound, often used to study vowels. Creates a Formant object.";
	if (actionIsStandardAnalysis (action, title, U"To Spectrogram"))
		return U"Show how the sound's frequency content changes over time. Creates a Spectrogram object.";
	if (actionIsStandardAnalysis (action, title, U"To Intensity"))
		return U"Measure how loud the sound is over time. Creates an Intensity object.";
	if (str32equ (title, U"To TextGrid...") && (actionIsForClass (action, U"Sound") || actionIsForClass (action, U"LongSound")))
		return U"Create a separate, empty TextGrid with the selected sound's time span. You choose its annotation tiers in the dialog.";
	if (Melder_startsWith (title, U"To TextGrid (silences)"))
		return U"Analyse the selected Sound and create a TextGrid that marks sounding and silent intervals automatically.";
	if (Melder_startsWith (title, U"To TextGrid (speech activity"))
		return U"Analyse the selected Sound and create a TextGrid marking likely speech and non-speech intervals.";
	if (Melder_startsWith (title, U"Filter (pass Hann band)") && actionIsForClass (action, U"Sound"))
		return U"Keep a chosen range of frequencies and make the other frequencies quieter, with smooth edges around the range. Creates a new Sound.";
	if (Melder_startsWith (title, U"Filter (stop Hann band)") && actionIsForClass (action, U"Sound"))
		return U"Make a chosen range of frequencies quieter while keeping frequencies outside it, with smooth edges. Creates a new Sound.";
	if (Melder_startsWith (title, U"Filter (pass Hann band)") && actionIsForClass (action, U"Spectrum"))
		return U"Modify the selected Spectrum to keep a chosen frequency band, with smooth transitions at its edges.";
	if (Melder_startsWith (title, U"Filter (stop Hann band)") && actionIsForClass (action, U"Spectrum"))
		return U"Modify the selected Spectrum to reduce a chosen frequency band, with smooth transitions at its edges.";
	if (Melder_startsWith (title, U"Filter with one formant (in-place)") && actionIsForClass (action, U"Sound"))
		return U"Emphasize one resonance in the selected Sound itself; this in-place command does not create a new Sound.";
	if (str32equ (title, U"Draw..."))
		return U"Draw the selected object in the Picture window. The dialog lets you choose how it is drawn.";
	if (Melder_startsWith (title, U"Draw "))
		return Melder_cat (U"Draw “", title + 5, U"” in the Picture window using the selected object.");
	if (Melder_startsWith (title, U"Draw"))
		return U"Draw the selected object in the Picture window. Choose a command here to decide what to display.";
	if (Melder_startsWith (title, U"Query"))
		return U"Measure or report information about the selected object. These commands usually show a result in the Info window.";
	if (Melder_startsWith (title, U"Modify"))
		return U"Change the selected object, such as its timing, samples, or values. Open this menu to choose a change.";
	if (Melder_startsWith (title, U"Annotate"))
		return U"Create or work with labels and boundaries for the selected sound.";
	if (Melder_startsWith (title, U"Analyse periodicity"))
		return U"Study repeating patterns in the sound, including pitch, pulses, and voice quality.";
	if (Melder_startsWith (title, U"Analyse spectrum"))
		return U"Study the sound's frequencies, including spectrograms and spectral measurements.";
	if (Melder_startsWith (title, U"Manipulate"))
		return U"Change the sound's pitch or duration using a Manipulation object.";
	if (Melder_startsWith (title, U"Convert"))
		return U"Create another kind of object from the selected one.";
	if (Melder_startsWith (title, U"Filter") && actionIsForClass (action, U"Sound"))
		return U"Change the balance of frequencies in the sound, for example to emphasize or reduce a frequency range.";
	if (Melder_startsWith (title, U"Combine") && title [7] == U' ' && (title [8] == U'>' || title [8] == U'-'))
		return U"Use selected objects together to create a new object.";
	if (str32equ (title, U"Combine to stereo"))
		return U"Put the selected Sounds into one multichannel Sound. The top selected mono Sound becomes the first channel; two inputs give left and right stereo channels.";
	if (str32equ (title, U"Combine into SoundList"))
		return U"Collect the selected Sounds in a SoundList. The sounds stay separate inside one container instead of being mixed or joined in time.";
	if (str32equ (title, U"Combine into SoundSet"))
		return U"Collect the selected Sounds in a SoundSet so you can work with them as one set while retaining the individual sounds.";
	if (str32equ (title, U"Concatenate") && actionIsForClass (action, U"Sound"))
		return U"Join the selected Sounds end to end in their Objects list order, creating one longer Sound.";
	if (str32equ (title, U"Concatenate recoverably"))
		return U"Join the selected Sounds end to end and create a TextGrid marking where each original Sound begins and ends, so the parts can be identified later.";
	if (str32equ (title, U"Concatenate with overlap..."))
		return U"Join selected Sounds end to end with short overlapping transitions. The dialog lets you choose the overlap duration.";
	if (str32equ (title, U"Convolve..."))
		return U"Convolve two selected Sounds to create a new Sound. This combines their waveforms as in applying an impulse response.";
	if (str32equ (title, U"Cross-correlate..."))
		return U"Compare two selected Sounds at different time offsets to find how closely their waveforms match.";
	if (str32equ (title, U"To CrossCorrelationTable (combined)..."))
		return U"Measure cross-correlations between the channels of two selected Sounds and create a CrossCorrelationTable.";
	if (str32equ (title, U"To DTW..."))
		return U"Compare the timing of two selected Sounds with dynamic time warping and create an alignment object.";
	if (str32equ (title, U"To ParamCurve"))
		return U"Make a ParamCurve from two selected Sounds, using one Sound for each coordinate over time.";
	if (Melder_startsWith (title, U"Get "))
		return Melder_cat (U"Measure “", title + 4, U"” for the selected object. Praat reports the result in the Info window.");
	if (Melder_startsWith (title, U"To "))
		return Melder_cat (U"Create “", title + 3, U"” from the selected object. A command ending in ... opens a settings dialog first.");
	if (Melder_startsWith (title, U"Extract "))
		return Melder_cat (U"Take “", title + 8, U"” out of the selected object and create a separate object.");
	if (Melder_startsWith (title, U"Draw "))
		return Melder_cat (U"Draw “", title + 5, U"” in the Picture window using the selected object.");
	return Melder_cat (U"Use “", title, U"” with the selected ", action -> class1 ? action -> class1 -> className : U"object",
		U". A command ending in ... opens a settings dialog before it runs.");
}

conststring32 praat_actionGuide (Praat_Command action) {
	if (const auto *help = soundHelp (action); help && help -> guide) return help -> guide;
	conststring32 title = action -> title.get();
	if (Melder_startsWith (title, U"Filter (pass Hann band)") && actionIsForClass (action, U"Spectrum"))
		return U"What happens in Praat? This command changes the selected Spectrum in place. A Spectrum represents the frequencies of a sound; it is not itself a Sound object.\n"
			U"What is a pass band? It is the range of frequencies kept stronger while frequencies outside it are reduced. From frequency and To frequency define its edges in hertz (Hz).\n"
			U"Why Hann? Praat uses a smooth transition at each edge instead of an abrupt cut, which helps avoid ringing. Smoothing (Hz) controls the width of those transitions.\n"
			U"How do I use it? Select a Spectrum, enter the band edges and smoothing, and confirm. Inspect the changed Spectrum; use Spectrum: To Sound if you need to hear a Sound made from it. Make a copy first if you want the original Spectrum unchanged.";
	if (Melder_startsWith (title, U"Filter (stop Hann band)") && actionIsForClass (action, U"Spectrum"))
		return U"What happens in Praat? This command changes the selected Spectrum in place, reducing a frequency range while keeping frequencies outside it stronger.\n"
			U"What is a stop band? It is the range between From frequency and To frequency that Praat reduces. The values are in hertz (Hz).\n"
			U"Why Hann? Smooth edges reduce the ringing that a sudden frequency cutoff can cause. Smoothing (Hz) sets the transition width.\n"
			U"How do I use it? Select a Spectrum, set the band and smoothing, and confirm. Inspect the changed Spectrum. If you need the original too, copy it before filtering; use Spectrum: To Sound to hear a Sound made from it.";
	if (Melder_startsWith (title, U"Filter (pass Hann band)") && actionIsForClass (action, U"Sound"))
		return U"What is a filter? A filter changes how strong different parts of a sound are. A sound contains slow and fast vibrations, heard as lower and higher frequencies.\n"
			U"Why filter? To focus on one frequency range, reduce unwanted sound outside it, or study one part of a complex sound.\n"
			U"What is a band? It is a range between two frequencies. A pass band keeps that range; frequencies outside it become quieter.\n"
			U"What is Hz? Hertz counts vibrations per second. A higher Hz value means a higher frequency.\n"
			U"Why Hann? A sudden cut at the band edges can add unwanted ringing. The Hann-shaped transition fades gradually, making the edges smoother. It is useful when splitting a sound into neighbouring bands. It is one sensible choice, not a universal best choice.\n"
			U"What does Smoothing do? It sets how wide the gradual transition is around each edge. More smoothing makes a gentler, less exact boundary.\n"
			U"How do I use it? Select a Sound, set From frequency and To frequency in Hz, choose Smoothing, and press OK. Praat creates a new Sound; listen to it beside the original. Adjust the range if it removes too much or too little. Use the stop version when you want to reduce the band instead.";
	if (Melder_startsWith (title, U"Filter (stop Hann band)") && actionIsForClass (action, U"Sound"))
		return U"What is a filter? It changes the strength of parts of a sound according to their frequency, from low to high.\n"
			U"What does stop band mean? The range between From frequency and To frequency is reduced; frequencies outside the range are kept. Use it to reduce a troublesome frequency region.\n"
			U"Why Hann? The smooth Hann-shaped edges avoid the abrupt cut that can cause ringing. Choose the pass version when you want to keep the band instead.\n"
			U"How do I use it? Select a Sound, choose the lower and upper frequencies in Hz, set Smoothing for the edge width, and press OK. Praat creates a new Sound so you can compare it with the original.";
	if (Melder_startsWith (title, U"Filter (formula)"))
		return U"What is a filter? It changes the balance of frequencies in a sound. Frequency means how quickly a vibration repeats; Hz means repetitions per second.\n"
			U"What is a formula here? Praat first represents the sound as a Spectrum. Your expression changes that Spectrum at each frequency x, then Praat converts it back to a Sound.\n"
			U"Why use this one? It gives precise control when a ready-made filter does not describe the change you want. Abrupt changes can cause ringing in the result.\n"
			U"How do I use it? Select a Sound, edit the formula in the dialog, and press OK. In the formula, x is frequency in Hz and self is the current spectral value. Start with a copy or a simple example, then listen to the new Sound and compare it with the original.";
	if (Melder_startsWith (title, U"Filter (one formant)"))
		return U"What is a formant? It is a frequency region strengthened by resonance, much like a musical instrument's body strengthens some notes. In speech, vocal-tract resonances help shape vowels.\n"
			U"What does this filter do? It emphasizes one resonance near the frequency you choose. Bandwidth controls how broad that emphasized region is, in Hz.\n"
			U"Why use it? To model or hear the effect of one resonance. It is a specialized effect, so it may change the sound markedly.\n"
			U"How do I use it? Select a Sound, enter the centre Frequency and positive Bandwidth, and press OK. Praat makes a new Sound; listen and compare it with the original. The in-place version changes the selected Sound instead.";
	if (Melder_startsWith (title, U"Filter (pre-emphasis)"))
		return U"What is pre-emphasis? It boosts higher frequencies relative to lower ones. Frequency is the rate of vibration, measured in Hz.\n"
			U"Why use it? Higher parts of speech can be weak compared with lower parts. Boosting them can make some spectral details easier to analyse. The result can sound thinner or noisier.\n"
			U"What does From frequency mean? It is the point that controls where this rising emphasis begins; it is not a hard cutoff.\n"
			U"How do I use it? Select a Sound, choose the From frequency in Hz, and press OK. Praat creates a new Sound. Compare its spectrum and playback with the original; de-emphasis makes the opposite kind of change.";
	if (Melder_startsWith (title, U"Filter (de-emphasis)"))
		return U"What is de-emphasis? It reduces higher frequencies relative to lower ones. Frequency is how quickly a sound vibration repeats, measured in Hz.\n"
			U"Why use it? To reverse pre-emphasis or soften an over-bright sound. It can also remove wanted high-frequency detail, so compare carefully.\n"
			U"What does From frequency mean? It controls where the gradual reduction starts; it is not a hard cutoff.\n"
			U"How do I use it? Select a Sound, enter the From frequency, and press OK. Praat makes a new Sound. Listen to both versions and inspect their spectra if the difference is hard to hear.";
	if (Melder_startsWith (title, U"Filter (gammatone)"))
		return U"What is a gammatone filter? It picks out a region of frequencies using a response shaped to resemble one part of the hearing system's frequency analysis.\n"
			U"What are centre frequency and bandwidth? Centre frequency is the middle of the region, in Hz. Bandwidth tells how broadly the filter responds around it; a larger bandwidth includes more nearby frequencies.\n"
			U"Why use it? To study what a narrow frequency region contributes to a recording, especially in hearing-related analyses. It is a model, not an exact simulation of hearing.\n"
			U"How do I use it? Select a Sound, enter a positive centre frequency and bandwidth, and press OK. Praat makes a new filtered Sound. Listen to it and compare with the original or with other centre frequencies.";
	if (Melder_startsWith (title, U"Filter with one formant (in-place)") && actionIsForClass (action, U"Sound"))
		return U"What happens in Praat? Praat filters the selected Sound itself to emphasize one resonance. Unlike Filter (one formant), this version changes the object in Objects instead of adding a new Sound.\n"
			U"What are Frequency and Bandwidth? Frequency places the resonance in hertz (Hz); Bandwidth controls how wide it is. A formant is a strengthened frequency region, useful for modelling vowel-like resonances.\n"
			U"Why use this version? It avoids a second object when you deliberately want to change the selected Sound. If you want to compare with the original, use Copy first or choose the non-in-place filter.\n"
			U"How do I use it? Select a Sound, enter Frequency and Bandwidth, confirm, and listen again. The same object now contains the filtered samples; save or export separately if you need a disk file.";
	if (Melder_startsWith (title, U"Reduce noise"))
		return U"What is noise reduction? It tries to make unwanted background sound quieter while keeping the wanted recording. It can also remove wanted detail or leave watery-sounding artifacts.\n"
			U"Why mark a noise time range? A stretch containing only the unwanted sound helps Praat estimate what to reduce elsewhere. Seconds (s) measure time; Hz measures frequency.\n"
			U"What do the other settings mean? Window length controls how much time is examined at once. The filter frequency range limits where the reduction acts. Smoothing bandwidth softens frequency boundaries. Noise reduction in dB controls the strength; a more negative value asks for more reduction.\n"
			U"How do I use it? Select a Sound, identify a short noise-only stretch, enter its start and end times, review the frequency range and reduction amount, and press OK. Listen to the new Sound beside the original, especially quiet speech.";
	if (Melder_startsWith (title, U"To Sound (derivative)"))
		return U"What is a derivative? It describes how quickly a value changes. For a Sound waveform, this makes rapid changes more prominent. It does not mean a new recording of the same event.\n"
			U"Why use it? To inspect or process changes in the waveform rather than its original amplitude. The result can emphasize high-frequency noise.\n"
			U"What do the settings mean? Low-pass frequency limits very high frequencies, smoothing softens that limit, and New absolute peak can scale the result's maximum size; zero leaves it unscaled.\n"
			U"How do I use it? Select a Sound, choose the frequency and smoothing values, decide whether to scale the peak, and press OK. Inspect the new Sound and compare it with the original.";
	if (str32equ (title, U"Combine to stereo"))
		return U"What happens in Praat? Praat creates a new Sound with one channel for each selected mono Sound. Two channels make stereo: the higher selected object in the Objects list becomes the left channel, and the lower one the right. With more than two inputs, the result has more than two channels.\n"
			U"What is a channel? It is one separate stream of sound samples within a Sound object. This command places streams side by side; it does not join clips one after another or mix them into one stream.\n"
			U"Why use it? To package synchronized recordings, such as separate left and right microphone signals, into a single multichannel Sound.\n"
			U"How do I use it? Select the mono Sounds in the Objects list, check their order and timing, then choose Combine to stereo. Inspect and listen to the new Sound; the originals remain in the list.";
	if (str32equ (title, U"Combine into SoundList"))
		return U"What is a SoundList? It is one object that holds several separate Sounds in a list. Each Sound keeps its own samples.\n"
			U"Why use it? To keep a collection together for commands that work on lists, without mixing the Sounds or changing their timing.\n"
			U"How do I use it? Select the Sounds and choose this command. Look for the new SoundList in Objects; the original Sounds remain available.";
	if (str32equ (title, U"Combine into SoundSet"))
		return U"What is a SoundSet? It is a container for several Sounds, kept as separate members of one object.\n"
			U"Why use it? Some Praat commands accept a SoundSet when you want to work with a collection of recordings.\n"
			U"How do I use it? Select the Sounds and choose this command. Praat creates a SoundSet in Objects; it does not merge their waveforms into one recording.";
	if (str32equ (title, U"Concatenate") && actionIsForClass (action, U"Sound"))
		return U"What does concatenate mean? Join things one after another. Here, the first Sound plays, then the next, making one longer Sound.\n"
			U"Why use it? To make one recording from several clips, such as consecutive speech segments.\n"
			U"How do I use it? Select the Sounds in the Objects list and choose this command. Check their order in the list before running it. The join may be audible if the clip edges do not match; use the overlap option for smoother transitions.";
	if (str32equ (title, U"Concatenate recoverably"))
		return U"What does recoverably mean? Praat also makes a TextGrid that marks the original pieces, so you can see where each piece starts and ends in the joined Sound.\n"
			U"Why use it? Choose it when you may need to find or extract the original parts later.\n"
			U"How do I use it? Select Sounds in the desired order, run the command, and inspect the new Sound together with its TextGrid. A TextGrid is a time-aligned track of boundaries and labels.";
	if (str32equ (title, U"Concatenate with overlap..."))
		return U"What is an overlap? The end of one clip and start of the next play together briefly instead of meeting at a sharp boundary.\n"
			U"Why use it? It can make a join less noticeable when the clip ends differ. A very long overlap may blur the content.\n"
			U"How do I use it? Select the Sounds in the intended order, choose the overlap duration in the dialog, and press OK. Listen around each join and adjust the duration if needed.";
	if (str32equ (title, U"Convolve..."))
		return U"What is convolution? It combines two waveforms so that one can act like a response applied to the other. For example, a short impulse response can add a room-like effect.\n"
			U"Why use it? For signal processing or to apply a measured response.\n"
			U"How do I use it? Select two Sounds, open the dialog, choose its settings, and create a new Sound. Listen and inspect the result, since the level and duration can change.";
	if (str32equ (title, U"Cross-correlate..."))
		return U"What is cross-correlation? It compares two waveforms while sliding one past the other. A strong peak means they match well at that time offset.\n"
			U"Why use it? To estimate delay or find similar patterns in two recordings.\n"
			U"How do I use it? Select two Sounds, choose the settings, and inspect the new result. The position of a peak tells you an offset; it is not itself proof that the recordings mean the same thing.";
	if (str32equ (title, U"To CrossCorrelationTable (combined)..."))
		return U"What is a CrossCorrelationTable? It stores numbers describing how strongly channels vary together.\n"
			U"Why use it? To compare relationships between channels across selected Sounds.\n"
			U"How do I use it? Select two Sounds, review the settings, and inspect the new table in Objects.";
	if (str32equ (title, U"To DTW..."))
		return U"What is DTW? Dynamic time warping finds a path between two recordings that may run at different speeds.\n"
			U"Why use it? To compare timing or align similar recordings.\n"
			U"How do I use it? Select two Sounds, choose settings in the dialog, and inspect the new alignment object. It describes a possible match, not a change to the original recordings.";
	if (str32equ (title, U"To ParamCurve"))
		return U"What is a ParamCurve? It plots values from one Sound against values from another at corresponding times, giving a path through a two-dimensional space.\n"
			U"Why use it? To see how two signals vary together.\n"
			U"How do I use it? Select two Sounds and run this command, then inspect or draw the new ParamCurve.";
	if (str32equ (title, U"View & Edit"))
		return U"What happens in Praat? This button opens the editor suited to the selected object. A Sound opens a Sound editor with its waveform on a time axis; a TextGrid opens an editor with labeled tiers. Selecting both a Sound and its TextGrid lets you see and hear the sound while placing labels. The object remains in the Objects list.\n"
			U"Why did Praat make a separate editor? The Objects window manages whole objects and offers commands for them. An editor lets you work at a particular moment or stretch of time: listen, zoom, select a region, inspect analyses, and edit where that object type allows it.\n"
			U"What am I looking at? In a Sound editor, time runs from left to right and the waveform shows how the recorded signal changes. Praat can display spectrogram, pitch, formants, and intensity alongside it; these views help you inspect the sound but do not by themselves replace the Sound in Objects. In a TextGrid editor, horizontal tiers hold time-aligned boundaries and labels.\n"
			U"How do I use it? Select the object in Objects and click View & Edit. In the editor, click or drag across time to choose a point or interval, then use its menus or controls. Close the editor to return to Objects. If you need a disk file, use a Save command; opening an editor alone does not save one.";
	if (str32equ (title, U"Play") && actionIsForClass (action, U"Sound"))
		return U"What happens in Praat? Praat sends the selected Sound objects to your audio output in their Objects list order. Playback does not change their samples or create a new object.\n"
			U"Why listen here? It is a quick check before measuring, filtering, or editing. Hearing the recording helps you judge whether an analysis result makes sense.\n"
			U"How do I use it? Select one or more Sounds on the left and click Play. To hear only a time span, open one Sound with View & Edit, select the interval, and play it in the editor. Check your system audio settings if you hear nothing.";
	if (str32equ (title, U"Play") && actionIsForClass (action, U"PointProcess"))
		return U"What happens in Praat? Praat turns the selected PointProcess's time points into a short pulse-train Sound for playback. This lets you hear the timing of those points; it does not play an original recording.\n"
			U"What is a PointProcess? It is a list of moments in time, often used for glottal pulses or other events. The spacing between points determines the rhythm you hear.\n"
			U"How do I use it? Select a PointProcess and click Play. To hear an underlying recording, select its Sound object instead.";
	if (str32equ (title, U"Play"))
		return U"What happens in Praat? Praat makes an audible representation of the selected object's data. Depending on the type, this may be synthesized rather than an original recording. Playback does not save a Sound file.\n"
			U"Why use it? Listening can reveal timing or synthesis behavior that numbers alone do not show.\n"
			U"How do I use it? Select the object and click Play. Inspect the object type in the left list to understand what Praat is turning into audio.";
	if (actionIsStandardAnalysis (action, title, U"To Pitch"))
		return U"What happens in Praat? Praat analyses the selected Sound across time and creates a separate Pitch object in Objects. The pitch curve shown inside View & Edit is a display; this command gives you an object you can edit, query, draw, or save.\n"
			U"What is pitch? It is the perceived highness or lowness of a repeating sound. For voiced speech, Praat estimates the fundamental frequency of vocal-fold vibration in hertz (Hz), cycles per second.\n"
			U"Which method should I choose? Praat's manual recommends filtered autocorrelation for typical intonation and vocal-fold frequency work, and raw cross-correlation for voice analysis. The methods can give different answers.\n"
			U"How do I use it? Select a Sound, choose a method, review the pitch floor and top for the speaker, then inspect the new Pitch object. Compare the curve with listening and the editor view; silence, noise, and settings can produce errors.";
	if (actionIsStandardAnalysis (action, title, U"To Formant"))
		return U"What happens in Praat? Praat estimates resonances across the selected Sound and creates a separate Formant object in Objects. The red formant marks in View & Edit are an editor analysis; this command creates an object you can query, draw, or save.\n"
			U"What is a formant? A formant is a frequency region strengthened by the vocal tract's resonances. The first few formants are especially useful when studying vowels.\n"
			U"Why do settings matter? The maximum formant frequency and analysis method affect which peaks Praat follows. A track can be wrong even when it looks smooth.\n"
			U"How do I use it? Select a Sound, review the formant ceiling and other settings, and inspect the new Formant object. Compare its values with the spectrogram and what you hear.";
	if (actionIsStandardAnalysis (action, title, U"To Spectrogram"))
		return U"What happens in Praat? Praat creates a separate Spectrogram object from the selected Sound. In View & Edit, you can also see a spectrogram without creating a new object; use this command when you need an object to query, draw, or save.\n"
			U"What am I looking at? Time runs left to right and frequency runs upward. Darker areas mean more energy at those frequencies and times. The spectrogram is an analysis picture, not another recording.\n"
			U"Why change the window length? A short window shows rapid events more clearly; a longer window separates nearby frequencies better. The choice depends on what you want to inspect.\n"
			U"How do I use it? Select a Sound, review settings such as window length and frequency range, then inspect the new Spectrogram in Objects. Compare with the editor display while listening to the Sound.";
	if (actionIsStandardAnalysis (action, title, U"To Intensity"))
		return U"What happens in Praat? Praat measures how the selected Sound's strength changes over time and creates a separate Intensity object. The intensity curve in View & Edit is an analysis display; this command makes an object you can query, draw, or save.\n"
			U"What is intensity here? Praat reports an estimated level in decibels (dB). Larger values mean a stronger recorded signal, but recording level and microphone distance affect the numbers.\n"
			U"Why does Pitch floor appear? Praat averages over time so the curve does not rise and fall with every vocal-fold pulse. A lower floor smooths over a longer stretch; a higher one shows faster changes.\n"
			U"How do I use it? Select a Sound, review Pitch floor and Time step, create the Intensity object, and compare its curve with the Sound. Check the units and recording conditions before comparing different files.";
	if (str32equ (title, U"To TextGrid...") && (actionIsForClass (action, U"Sound") || actionIsForClass (action, U"LongSound")))
		return U"What happens in Praat? Praat creates a separate, empty TextGrid whose start and end times match the selected Sound or LongSound. It does not attach labels to the audio samples or change the recording.\n"
			U"What are tiers? They are parallel tracks for different kinds of labels, such as words and phonemes. Interval tiers label stretches between boundaries; point tiers label moments.\n"
			U"Why create it from a Sound? Praat copies the sound's time span for you, so the annotation and audio line up when you open them together.\n"
			U"How do I use it? Name your tiers in the dialog and identify any point tiers. Then select both the Sound and new TextGrid in Objects and click View & Edit to place boundaries and labels while listening. Save the TextGrid separately when finished.";
	if (Melder_startsWith (title, U"To TextGrid (silences)") && actionIsForClass (action, U"Sound"))
		return U"What happens in Praat? Praat creates a TextGrid with intervals labeled as silent or sounding, based on an analysis of the selected Sound. It does not transcribe words or change the audio.\n"
			U"Why use it? It gives you a starting map of pauses and active sound that you can inspect and correct by hand. Background noise can make the boundaries imperfect.\n"
			U"How do I use it? Select a Sound, review the noise and threshold settings, and run the command. Select the Sound and resulting TextGrid together, open View & Edit, listen around the boundaries, and adjust labels or times if needed.";
	if (Melder_startsWith (title, U"To TextGrid (speech activity") && actionIsForClass (action, U"Sound"))
		return U"What happens in Praat? Praat creates a separate TextGrid marking stretches judged likely to contain speech. Different commands here use different detection methods, so their results may disagree.\n"
			U"Why use it? It can speed up locating speech in a long or noisy recording, but it is not a transcript and may miss quiet speech or mark other sounds as speech.\n"
			U"How do I use it? Select a Sound, read the method and settings, run it, then open the new TextGrid together with the Sound. Listen to the marked intervals and correct them before relying on the labels.";
	if (Melder_startsWith (title, U"Filter"))
		return U"What happens in Praat? Commands in this section process the selected Sound's samples and normally put a new Sound in Objects. They change which frequencies are strong or weak in that new version. The original stays available for comparison.\n"
			U"What is a frequency? It is how quickly a vibration repeats. Praat asks for many frequency settings in hertz (Hz), or repetitions per second. Lower Hz often sounds lower; higher Hz often sounds higher.\n"
			U"Why filter a Sound? You may want to reduce unwanted noise, focus on one frequency region, or prepare a recording for an analysis. A filter can also remove wanted sound, so listening matters.\n"
			U"How do I use this menu? Select a Sound, hover over a specific filter to learn what it keeps or reduces, then open its settings. After Praat creates the result, select and play both versions to compare them.";
	if (Melder_startsWith (title, U"Combine"))
		return U"What happens in Praat? These commands take the objects you selected together in the left list and create a new result. The choice matters: a stereo Sound places inputs in separate channels, Concatenate joins clips in time, and SoundList or SoundSet keeps them as separate members of one container.\n"
			U"Why combine objects? Praat can then play or analyse them as one result, or keep a related collection together. Combining does not always mean mixing the audio.\n"
			U"How do I choose? Select two or more compatible objects in Objects, hover over the specific command to see its result, and run it. Inspect the new object in the list and listen if it is a Sound.";
	if (Melder_startsWith (title, U"Analyse periodicity"))
		return U"What is periodicity? It means a pattern repeats, such as the regular vibrations of a voiced sound. Pitch and pulses are related measurements.\n"
			U"Why study it? To examine intonation, voicing, or voice quality.\n"
			U"How do I use it? Select a Sound and open a command in this menu. Read its settings, inspect the result, and compare it with what you hear; noise and silence can mislead an estimate.";
	if (Melder_startsWith (title, U"Analyse spectrum"))
		return U"What is a spectrum? It shows how much sound energy lies at different frequencies. A spectrogram shows that information changing over time.\n"
			U"Why study it? To inspect harmonics, resonances, or noise that make up a sound.\n"
			U"How do I use it? Select a Sound, choose a command, review its time and frequency settings, and inspect the new object or reported value.";
	if (Melder_startsWith (title, U"Annotate"))
		return U"What is annotation? It means adding labels or boundaries to a timeline, for example marking where words begin and end.\n"
			U"Why use it? Labels make it easier to find, compare, and measure parts of a recording.\n"
			U"How do I use it? Select a Sound or TextGrid, choose an annotation command, and inspect the new or changed timeline. A TextGrid stores labels separately from the audio.";
	if (Melder_startsWith (title, U"Manipulate"))
		return U"What is manipulation? It means changing an aspect of the sound, commonly pitch or duration. Praat can store editable changes in a Manipulation object.\n"
			U"Why use it? To experiment with how speech sounds when timing or pitch changes.\n"
			U"How do I use it? Select a Sound, make a Manipulation object if needed, edit its controls, then resynthesize and listen to the new Sound.";
	if (Melder_startsWith (title, U"Get ") || Melder_startsWith (title, U"Query"))
		return U"What is a query? It asks Praat to calculate or report a property of the selected object, such as a time, value, or measurement.\n"
			U"Why use it? To get a number you can compare, record, or use in further analysis.\n"
			U"How do I use it? Select an object, choose the measurement, fill in any requested time or range, and read the answer in the Info window. Check units and settings before comparing results.";
	if (Melder_startsWith (title, U"To ") || Melder_startsWith (title, U"Convert"))
		return U"What is conversion? Praat makes a new kind of object from the selected one. For example, it can turn a Sound into measurements across time.\n"
			U"Why use it? Different objects let you inspect or edit different aspects of the same data.\n"
			U"How do I use it? Select the source object, choose a command, review any settings, then find the new object in the Objects list. Open its editor or Info window to understand the result.";
	if (Melder_startsWith (title, U"Extract "))
		return U"What does extract mean? It means taking a part or property out of the selected object and making a separate object from it.\n"
			U"Why use it? To focus on a time segment, channel, or measurement while keeping the source available.\n"
			U"How do I use it? Select the object, choose what to extract, review any range or settings, and inspect the new object in the list.";
	if (Melder_startsWith (title, U"Draw"))
		return U"What is the Picture window? It is where Praat draws figures you can inspect, print, or export. Drawing does not change the selected object.\n"
			U"Why draw? To show a waveform, measurement, annotation, or other result visually.\n"
			U"How do I use it? Select an object, choose a drawing command, set the view or range if prompted, then look in the Picture window.";
	if (Melder_startsWith (title, U"Modify"))
		return U"What does modify mean? It changes data or properties of the selected object.\n"
			U"Why use it? To correct, transform, or prepare an object for the next step.\n"
			U"How do I use it? Select the object, open the command, read its settings, and inspect the result. Make a copy first if you want to keep the original state.";
	return U"What happens in Praat? This command is available because the selected object type supports it. Praat works with objects in memory: a Sound holds audio samples, a TextGrid holds timed labels, and analysis objects hold measurements. The command uses the selected object and may report a value, change it, draw it, or create another object.\n"
		U"Why is it on the right? Praat changes this action list to match what you selected on the left. A command shown for a Sound may disappear when you select a different object type.\n"
		U"How do I use it? Check the selected object, choose the command, and read any dialog before pressing OK. A name ending in ... means Praat will ask for settings. Afterward, look for a new object in Objects, a value in Info, or a drawing in Picture, depending on the command.";
}
