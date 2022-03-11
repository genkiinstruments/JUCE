/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

class FileChooser::Native    : public FileChooser::Pimpl,
                               private Timer
{
public:
    Native (FileChooser& fileChooser, int flags, FilePreviewComponent*)
        : owner (fileChooser),
          isDirectory         ((flags & FileBrowserComponent::canSelectDirectories)   != 0),
          isSave              ((flags & FileBrowserComponent::saveMode)               != 0),
          selectMultipleFiles ((flags & FileBrowserComponent::canSelectMultipleItems) != 0),
          warnAboutOverwrite  ((flags & FileBrowserComponent::warnAboutOverwriting)   != 0)
    {
    }

    ~Native() override
    {
        finish (true);
    }

    void runModally() override
    {
        // TODO
        finish (false);
    }

    void launch() override
    {
        startTimer (100);
    }

private:
    FileChooser& owner;
    bool isDirectory, isSave, selectMultipleFiles, warnAboutOverwrite;

    StringArray args;
    String separator;

    void timerCallback() override
    {
        // TODO
        {
            stopTimer();
            finish (false);
        }
    }

    void finish (bool shouldKill)
    {
        Array<URL> selection;

        if (! shouldKill)
        {
            owner.finished (selection);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Native)
};

bool FileChooser::isPlatformDialogAvailable()
{
   #if JUCE_DISABLE_NATIVE_FILECHOOSERS
    return false;
   #else
    return true;
   #endif
}

std::shared_ptr<FileChooser::Pimpl> FileChooser::showPlatformDialog(FileChooser& owner, int flags,
                                                                    FilePreviewComponent* preview)
{
    return std::make_shared<FileChooser::Native>(owner, flags, preview);
}

} // namespace juce
