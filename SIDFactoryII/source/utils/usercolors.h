#pragma once

#include "foundation/graphics/color.h"

namespace Foundation
{
	enum class Color : unsigned short;
	class Viewport;
}

namespace Utility
{
	enum class UserColor : unsigned short
	{
		TableDefault,
		TableHighlight,
		TableText,
		TableTextEditing,
		TableTextBackground,
		ButtonText,
		ButtonDefault,
		ButtonDefaultMouseOver,
		ButtonHighlight,
		ButtonHighlightMouseOver,
		FileSelectorBackground,
		FileSelectorListText,
		FileSelectorTextInput,
		FileSelectorCursorNoFocus,
		FileSelectorCursorFocusDefault,
		FileSelectorCursorFocusLoadSong,
		FileSelectorCursorFocusImportSong,
		FileSelectorCursorFocusSaveSong,
		FileSelectorCursorFocusSavePacked,
		MarkerListBackground,
		MarkerListText,
		MarkerListCursorNoFocus,
		MarkerListCursorFocus,
		SongListBackground,
		SongListEventPos,
		SongListValues,
		SongListCursorDefault,
		SongListCursorFocus,
		SongListPlaybackMarker,
		SongListLoopMarker,
		TrackBackground,
		TrackBackgroundMuted,
		TrackName,
		TrackNameSelected,
		TrackNameFocus,
		TrackNameSelectedFocus,
		TracksEventNumbersBackground,
		TracksEventNumbersDefault,
		TracksEventNumbersFocusLine,
		TracksEventNumbersHighlight,
		TracksEventNumbersHighlightFocusLine,
		TracksEventNumbersPlaybackMarker,
		SequenceError,
		SequenceInstrumentEmpty,
		SequenceInstrumentEmptyFocusLine,
		SequenceInstrumentTie,
		SequenceInstrumentTieFocusLine,
		SequenceInstrumentValue,
		SequenceInstrumentValueFocusLine,
		SequenceInstrumentValueSelectedHighlight,
		SequenceInstrumentValueSelectedHighlightFocusLine,
		SequenceCommandEmpty,
		SequenceCommandEmptyFocusLine,
		SequenceCommandValue,
		SequenceCommandValueFocusLine,
		SequenceCommandValueSelectedHighlight,
		SequenceCommandValueSelectedHighlightFocusLine,
		SequenceNoteEmpty,
		SequenceNoteEmptyFocusLine,
		SequenceNoteGateOn,
		SequenceNoteGateOnFocusLine,
		SequenceNoteValue,
		SequenceNoteValueFocusLine,
		SequenceNoteValueTied,
		SequenceNoteValueTiedFocusLine,
		SequenceNoteValueSelectedHighlight,
		SequenceNoteValueSelectedHighlightFocusLine,
		OrderlistEndLoop,
		OrderlistEndStop,
		OrderlistValue,
		OrderlistValueLoopMarker,
		OrderlistValueInput,
		DialogBackground,
		DialogHeader,
		DialogHeaderText,
		DialogText,
		DialogListSelectorCursor,
		DialogListSelectorCursorFocus,
		DialogOptimzerUsed,
		DialogOptimzerUnused,
		StatusBarText,
		StatusBarBackgroundStopped,
		StatusBarBackgroundStoppedMouseOverRegion,
		StatusBarBackgroundPlaying,
		StatusBarBackgroundPlayingMouseOverRegion,
		StatusBarBackgroundPlayingInput,
		StatusBarBackgroundPlayingInputMouseOverRegion,
		ScreenEditInfoRectBackground,
		ScreenEditInfoRectText,
		ScreenEditInfoRectTextTimePlaybackState,
		ScreenEditTableHeadline,
		ScreenEditTableHeadlineHotkeyLetter,

		DriverColorSet = 0x60,
		DriverColorJump,
		DriverColor02,
		DriverColor03,
		DriverColor04,
		DriverColor05,
		DriverColor06,
		DriverColor07,
		DriverColor08,
		DriverColor09,
		DriverColor0a,
		DriverColor0b,
		DriverColor0c,
		DriverColor0d,
		DriverColor0e,
		DriverColor0f,
		DriverColor10,
		DriverColor11,
		DriverColor12,
		DriverColor13,
		DriverColor14,
		DriverColor15,
		DriverColor16,
		DriverColor17
	};

	Foundation::Color ToColor(UserColor inColor);

	void SetUserColor(Foundation::Viewport& inViewport, UserColor inUserColor, Foundation::Color inColor);
	void SetUserColor(Foundation::Viewport& inViewport, UserColor inUserColor, unsigned int inARGB);
}