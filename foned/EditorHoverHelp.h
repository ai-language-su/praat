#ifndef _EditorHoverHelp_h_
#define _EditorHoverHelp_h_
#include "GuiHoverHelp.h"
namespace EditorHoverHelp {
inline constexpr conststring32 frequencyCursor =
 U"Selected frequency (Hz). This red number gives the cursor position on the Spectrum's frequency axis, not a pitch estimate.\n____________________________\n"
 U"What is Hz? Hertz means cycles per second. The Spectrum describes components at many frequencies; the cursor just marks one frequency, even if no peak is there.\n"
 U"How do I use it? Click to move the frequency cursor or drag to select a band. Band filtering and publishing commands use that selection. Move cursor to nearest peak can help locate a local spectral maximum.";
inline constexpr conststring32 timeCursor =
 U"Time cursor or selection boundary, in seconds from this Sound's time origin. It is a position, not the duration of the visible recording.\n____________________________\n"
 U"What does the red line mark? With a single cursor, the same time is used by the waveform and analysis panels. When an interval is selected, the red numbers mark its start and end.\n"
 U"How do I use it? Click to place the cursor or drag to select a time interval. The displayed analyses are estimates of the Sound around that time; zooming changes the view, not the audio.";
inline constexpr conststring32 frequencyStart =
 U"Lowest frequency currently visible in the Spectrum editor, in hertz (Hz). This is a display boundary.\n____________________________\n"
 U"What does it mean? Frequencies increase from left to right. Zero Hz is the constant, or DC, component. Zooming and scrolling can make the left boundary greater than zero.\n"
 U"Does it filter the Sound? No. This number shows the view's edge. Selecting a band and running a filter is a separate operation.";
inline constexpr conststring32 frequencyEnd =
 U"Highest frequency currently visible in the Spectrum editor, in hertz (Hz). It is not necessarily the cursor frequency or the highest peak.\n____________________________\n"
 U"Why 22050 Hz? A Sound sampled at 44100 samples per second can represent frequencies up to half that rate, called the Nyquist frequency. The Spectrum's actual upper limit depends on its source and sampling.\n"
 U"How do I change the view? Zoom or scroll. That changes the displayed range without removing frequency components.";
inline constexpr conststring32 timeStart =
 U"Start time of the visible window, in seconds. This may differ from the start of the complete Sound.\n____________________________\n"
 U"What is visible? Only the interval between the blue start and end numbers is shown in the plots. Audio outside it can still exist.\n"
 U"How do I use it? Use all, zoom or the scrollbar to change the view. For extracted or shifted Sounds, time zero need not be the object's actual start boundary.";
inline constexpr conststring32 timeEnd =
 U"End time of the visible window, in seconds. This is a timeline position rather than the window's duration.\n____________________________\n"
 U"How is duration calculated? Visible part is the visible end minus the visible start. Total duration covers the complete Sound, including any audio outside the window.\n"
 U"How do I use it? Zoom or scroll to inspect another interval. Changing this view does not cut or stretch the recording.";
inline constexpr conststring32 frequencyVisible =
 U"Visible frequency span: the difference between the right and left frequency boundaries, in Hz.\n____________________________\n"
 U"What does Visible part mean? This much of the Spectrum is currently on screen. It is a frequency bandwidth, not elapsed time, pitch or the width of a spectral peak.\n"
 U"How do I use it? Compare with Total bandwidth to see whether you are zoomed in. Clicking this bar auditions a Sound made from the visible frequency band; the Spectrum itself is not changed by playback.";
inline constexpr conststring32 timeVisible =
 U"Visible time span: the visible end time minus the visible start time, in seconds. Zooming changes this span without changing the Sound.\n____________________________\n"
 U"Why can it be shorter than Total duration? Some of the Sound lies outside the current view. The blue end labels tell you which interval is visible.\n"
 U"How do I use it? Click this bar to play the visible interval, or use all and the zoom buttons to change what is shown.";
inline constexpr conststring32 frequencyTotal =
 U"Total bandwidth of this Spectrum: its full frequency-domain span in Hz, including components outside the current zoomed view.\n____________________________\n"
 U"What is bandwidth here? Highest domain frequency minus lowest domain frequency. This is the whole object's frequency range, not a filter's pass band or the width of one resonance.\n"
 U"How do I use it? Compare it with Visible part to understand your zoom. Clicking this bar auditions the full spectral range; all restores the full display.";
inline constexpr conststring32 timeTotal =
 U"Total duration of this Sound: its end boundary minus its start boundary, in seconds, including audio outside the visible window.\n____________________________\n"
 U"Why distinguish it from the view? You can zoom into a small part without shortening the recording. It also differs from a cursor's time position.\n"
 U"How do I use it? Click this bar to play the whole domain or use all to display it. The view's scrollbar and zoom do not alter the samples.";
inline constexpr conststring32 frequencySegment =
 U"Width in Hz of this frequency segment between the visible edge and/or selection markers. It is a difference between positions, not another selected frequency.\n____________________________\n"
 U"Why are several numbers shown? The cursor or selection divides the visible frequency range into segments. Their widths can be displayed on separate bars.\n"
 U"How do I use it? Clicking a segment auditions that frequency band as sound. To apply filtering, make a frequency selection and use the Spectrum menu.";
inline constexpr conststring32 timeSegment =
 U"Length in seconds of this segment between the visible edge and/or selection markers. It is an interval length, not a timestamp.\n____________________________\n"
 U"Why does this number change? Moving the cursor, selection or visible window changes the segment's endpoints. Several bars can divide the same view.\n"
 U"How do I use it? Click the segment bar to play that interval. Audio outside it remains in the Sound.";
inline constexpr conststring32 frequencySelection =
 U"Width of the selected frequency band, in Hz: selection end minus selection start.\n____________________________\n"
 U"How is it used? Spectrum band filtering, publishing and band playback use the frequency selection. The selected band's width is different from its centre frequency and from Total bandwidth.";
inline constexpr conststring32 timeSelection =
 U"Length of the selected time interval, in seconds. If a value in /s is shown in parentheses, it is the reciprocal of that interval length.\n____________________________\n"
 U"What is the reciprocal useful for? An interval of 0.01 s corresponds to 100 repetitions per second if interpreted as one cycle. This is a calculation from your selection, not an automatic estimate of the speaker's pitch.\n"
 U"How do I use it? Drag to choose an interval, then play or inspect it. Editing commands may act on the selection, so check its boundaries first.";
inline constexpr conststring32 spectrumRange =
 U"Upper or lower limit of the Spectrum's displayed power-density scale, in dB. The blue labels set the plotted vertical range; they are not both measurements at the cursor.\n____________________________\n"
 U"What are these decibels? Praat displays logarithmic spectral power density. The level describes how energy is distributed across frequencies, rather than the Sound editor's overall intensity curve. Negative dB values are valid on a relative logarithmic scale. Absolute physical interpretation requires calibrated audio.\n"
 U"How do I change this? Spectrum > Set dynamic range changes how many dB fit vertically. It changes the view, not the stored Spectrum or playback volume.";
inline constexpr conststring32 spectrumCursorLevel =
 U"Height of the red horizontal Spectrum cursor, in dB. This is the vertical coordinate you clicked, not automatically the spectrum's value at the selected frequency.\n____________________________\n"
 U"Why separate the two? A click can place the crosshair anywhere, including above or below the curve. The frequency cursor and level cursor define a location on the graph.\n"
 U"How do I use it? Click to move the crosshair. Move cursor to nearest peak snaps both coordinates to a local maximum. The red level and blue display-limit values have different roles.";
inline constexpr conststring32 spectrumPlot =
 U"Spectrum: frequency runs left to right, and the curve shows logarithmic spectral power density vertically. Peaks indicate stronger frequency components.\n____________________________\n"
 U"How is this different from the Sound waveform? This plot is organised by frequency in Hz, not recording time in seconds. It does not by itself show when a frequency occurred.\n"
 U"How do I use it? Click for a frequency/level crosshair or drag to select a frequency band. Play or publish a band to hear its contribution. Filtering in this editor changes the Spectrum object, so keep a copy when needed.";
inline constexpr conststring32 waveformRange =
 U"Waveform amplitude scale for this channel. These signed values describe sample amplitude, not pitch or decibels.\n____________________________\n"
 U"What do positive and negative mean? Opposite directions of the waveform around zero. A negative value is not negative loudness. Praat represents pressure amplitude in pascals when the recording is calibrated; ordinary file levels alone do not establish physical pressure.\n"
 U"Why does the range change? Depending on Sound scaling settings, zooming can rescale the display to the visible samples. This affects the drawing, not the audio level.";
inline constexpr conststring32 waveformZero =
 U"Zero waveform amplitude: the reference line between positive and negative sample values. It is not zero seconds or zero Hz.\n____________________________\n"
 U"Does touching zero mean silence? No. An oscillating sound crosses zero repeatedly. A sustained run of zero-valued samples is digital silence; one zero crossing can occur within a loud sound.";
inline constexpr conststring32 waveformCursor =
 U"Waveform amplitude at the time cursor in this channel, estimated between stored samples if necessary. It is one amplitude value, not averaged loudness.\n____________________________\n"
 U"How do I interpret it? Positive and negative values describe the waveform's sign. The point on the waveform and the readout refer to the same cursor time. Compare the scale limits for context, or use intensity/RMS when you need a level averaged over time.";
inline constexpr conststring32 waveformPlot =
 U"Sound waveform: time in seconds runs left to right; signed sample amplitude runs vertically for each channel.\n____________________________\n"
 U"What does its shape tell me? It shows how the recorded signal varies, with larger swings often corresponding to stronger signal levels. Pitch is the rate of repetition, not the waveform's height.\n"
 U"How do I use it? Click for a time cursor, drag for a selection, and zoom to inspect individual cycles or samples. In an editable Sound, sample edits affect the object in memory; a read-only or disk-backed area may not offer those edits. Showing analyses below it does not itself change the recording.";
inline constexpr conststring32 spectrogramRange =
 U"Lower or upper visible frequency bound of the spectrogram/formant panel, in Hz. This scale describes frequency, not time or amplitude.\n____________________________\n"
 U"What does 5000 Hz mean here? The plot's top is showing frequencies up to 5000 cycles per second. It is not a measured pitch of 5000 Hz. The lower bound can be 0 Hz.\n"
 U"How do I use it? Read time along the bottom and frequency on the left. Greater spectral energy appears darker in Classic, Light and Warm themes, and lighter in Dark. Spectrogram settings change this visible frequency range without altering the Sound.";
inline constexpr conststring32 spectrogramCursor =
 U"Frequency of the spectrogram's horizontal cursor, in Hz. It marks where you clicked vertically; it is not an automatically detected formant or pitch value.\n____________________________\n"
 U"How do I use it? Read a feature's approximate frequency from this marker and its time from the vertical cursor. The red dotted line is a position guide, while coloured pitch or formant tracks come from analysis. Frequency means cycles per second.";
inline constexpr conststring32 pitchRange =
 U"Lower or upper bound of the pitch display's frequency scale. Blue pitch uses its own scale on the right, separate from the spectrogram frequency scale on the left.\n____________________________\n"
 U"What is pitch here? An estimate of fundamental frequency (F0), the repetition rate of voiced sound, usually displayed in Hz. A top label of 800 Hz is a scale limit, not a claim that the voice has that pitch.\n"
 U"How do I use it? Read the blue curve against these bounds. Pitch settings control the analysis range and view settings. Silence, unvoiced speech and estimation errors can produce gaps or incorrect tracks.";
inline constexpr conststring32 pitchCursor =
 U"Estimated pitch at the time cursor, or average pitch over the selected interval. This is an analysis result, unlike a display-bound label.\n____________________________\n"
 U"What does it estimate? Fundamental frequency, the repetition rate of voiced sound, in the pitch units selected in settings. In Hz it means cycles per second. It is not the same as the spectrogram's clicked frequency.\n"
 U"How do I use it? Check the curve against listening and the waveform. The analysis can double or halve the true repetition frequency, and may be undefined in unvoiced regions.";
inline constexpr conststring32 intensityRange =
 U"Lower or upper limit of the green intensity display, in dB. These green range labels belong to the intensity curve, not the blue pitch or grayscale spectrogram.\n____________________________\n"
 U"What is intensity? A level estimated by averaging squared waveform amplitudes over a short time. Decibels use a logarithmic scale. Recording gain and microphone distance affect the result, and absolute sound-pressure interpretation requires calibration.\n"
 U"How do I use it? Read the green curve against these bounds. A 100 dB label is a display limit, not necessarily a level measured in this recording. Intensity settings change the display/analysis without changing sample amplitudes.";
inline constexpr conststring32 intensityCursor =
 U"Estimated intensity at the cursor, or an average over the selected time interval, in dB. This is a measured analysis value rather than a scale bound.\n____________________________\n"
 U"What do the extra symbols mean? For interval selections, a suffix identifies the averaging method: median, energy averaging, sones averaging or averaging dB. Different averaging methods can produce different results.\n"
 U"How do I use it? Compare equivalent time regions and recording conditions. Intensity is not directly perceived loudness, and an uncalibrated file does not establish absolute sound-pressure level.";
inline constexpr conststring32 analysisPlot =
 U"Derived analyses of the Sound share the time axis but can have different vertical scales. The spectrogram shows frequency energy; blue pitch estimates repetition rate; green intensity estimates level.\n____________________________\n"
 U"How do I read this? Frequency labels at the left belong to the spectrogram/formants. Blue labels on the right belong to pitch, and green dB labels belong to intensity. Their heights cannot be compared as though they were the same quantity.\n"
 U"What does derived mean? Calculated from the Sound with the current analysis settings. These displays are not separately saved objects. Use the analysis menus to control them or publish objects for separate querying and saving.";
inline constexpr conststring32 channel =
 U"Audio channel label. A channel is one stream of waveform samples, such as left or right in stereo. The speaker symbol indicates playback muting for that channel.\n____________________________\n"
 U"How do I use it? Use the channel controls to inspect or mute individual channels for listening. Muting playback does not erase the channel's stored samples. A channel is not necessarily one speaker; it depends on how the recording was made.";
inline constexpr conststring32 group =
 U"Group links selection and, when enabled in preferences, zoom/scroll among compatible editors with the same domain.\n____________________________\n"
 U"Why use it? Compare aligned recordings or analyses at the same position. This synchronises editor views; it does not mix Sounds, merge objects or guarantee their data are aligned.\n"
 U"How do I use it? Group compatible editors and move a cursor to see the shared selection. Editors with different domains, such as time and frequency, do not form one meaningful linked timeline.";
inline constexpr conststring32 hiddenTime =
 U"Duration of the Sound lying outside this side of the visible window, in seconds. This audio still exists; it is merely off screen.\n____________________________\n"
 U"How do I use it? Scroll or zoom out to see that interval. Clicking its playback bar plays the hidden interval. It is not the cursor's time or an extra analysed measurement.";
inline constexpr conststring32 hiddenFrequency =
 U"Width in Hz of the Spectrum lying outside this side of the visible window. These frequencies have not been deleted by zooming.\n____________________________\n"
 U"How do I use it? Scroll or zoom out to display the band. Clicking its playback bar auditions that band as sound; this is separate from applying a filter.";
inline constexpr conststring32 pitchLegend =
 U"Derived pitch: the blue curve estimates fundamental frequency, the repetition rate of voiced sound. Read it against the blue pitch scale, not the spectrogram scale.\n____________________________\n"
 U"What does derived mean? Praat calculates it from the Sound using current pitch settings. It can have gaps or octave errors. Showing it does not create a separate Pitch object in Objects or change the recording.";
inline constexpr conststring32 intensityLegend =
 U"Derived intensity: the green curve estimates recorded signal level over time in dB. Read it against its green scale.\n____________________________\n"
 U"How is it obtained? Praat averages squared waveform amplitudes over short intervals. Recording gain affects it; absolute sound-pressure interpretation requires calibration. This displayed analysis does not change the Sound or create a separate Intensity object.";
inline constexpr conststring32 spectrogramLegend =
 U"Derived spectrogram: a time-frequency view calculated from the Sound. Time runs horizontally and frequency vertically; shading shows spectral energy.\n____________________________\n"
 U"How do I use it? Read frequency from the left-hand Hz scale and adjust window length and dynamic range in Spectrogram settings to inspect different detail. The display is an analysis of the Sound, not a separate Spectrogram object automatically saved in Objects.";
inline constexpr conststring32 formantLegend =
 U"Derived formants: estimated resonance frequencies, commonly used to study vowels. They are different from pitch, the repetition frequency of voiced sound.\n____________________________\n"
 U"How do I use them? Compare the formant marks with the spectrogram and check Formants settings, especially the ceiling. The tracks are model estimates and can be wrong. Displaying them does not create a separate Formant object.";
inline constexpr conststring32 pulsesLegend =
 U"Derived pulses: estimated repeating excitation events in voiced sound, drawn as markers at times on the waveform. They are not the audio sample positions.\n____________________________\n"
 U"Why inspect them? Missing or extra pulses affect voice-quality and resynthesis work. Check their placement against the waveform and pitch settings. The displayed markers do not by themselves create a separate PointProcess object.";
inline conststring32 forLegend (conststring32 label) {
	if (! label || ! *label) return nullptr;
	if (str32str (label, U"spectrogram")) return spectrogramLegend;
	if (str32str (label, U"intensity")) return intensityLegend;
	if (str32str (label, U"pitch")) return pitchLegend;
	if (str32str (label, U"formant")) return formantLegend;
	if (str32str (label, U"pulses")) return pulsesLegend;
	if (str32str (label, U"sound")) return waveformPlot;
	return nullptr;
}
}
#endif
