/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2013 - Raw Material Software Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"

extern juce::JUCEApplicationBase* juce_CreateApplication(); // (from START_JUCE_APPLICATION)

namespace juce {

extern const char* const* juce_argv;  // declared in juce_core
extern int juce_argc;
extern std::unique_ptr<juce::ScopedJuceInitialiser_GUI> libraryInitialiser; // from juce_emscriptenMessaging

class EmscriptenComponentPeer;

// A singleton class that accepts mouse and keyboard events from browser
//   main thread and post them as messages onto the message thread.
// It is useful when main thread != message thread (-s PROXY_TO_PTHREAD).
class MainThreadEventProxy : public MessageListener
{
public:
    struct MouseEvent : public EmscriptenEventMessage
    {
        String type;
        int x, y;
        int which;
        bool isShiftDown, isCtrlDown, isAltDown;
        int wheelDelta;
    };

    struct KeyboardEvent : public EmscriptenEventMessage
    {
        String type;
        int keyCode;
        String key;
    };

    struct InputEvent : public EmscriptenEventMessage
    {
        EmscriptenComponentPeer* target;
        String type;
        String data;
    };

    static MainThreadEventProxy& getInstance()
    {
        if (globalInstance == nullptr)
        {
            globalInstance.reset (new MainThreadEventProxy());
        }
        return *globalInstance;
    }

private:
    void handleMessage (const Message& msg) override
    {
        if (dynamic_cast<const MouseEvent*>(& msg))
        {
            const MouseEvent* e = dynamic_cast<const MouseEvent*>(& msg);
            handleMouseEvent (*e);
        } else
        if (dynamic_cast<const KeyboardEvent*>(& msg))
        {
            const KeyboardEvent* e = dynamic_cast<const KeyboardEvent*>(& msg);
            handleKeyboardEvent (*e);
        } else
        if (dynamic_cast<const InputEvent*>(& msg))
        {
            const InputEvent* e = dynamic_cast<const InputEvent*>(& msg);
            handleInputEvent (*e);
        }
    }

    void handleMouseEvent (const MouseEvent& e);
    void handleKeyboardEvent (const KeyboardEvent& e);
    void handleInputEvent (const InputEvent& e);

    static std::unique_ptr<MainThreadEventProxy> globalInstance;
};

std::unique_ptr<MainThreadEventProxy> MainThreadEventProxy::globalInstance;

extern "C" void juce_mouseCallback(const char* type, int x, int y, int which,
    int isShiftDown, int isCtrlDown, int isAltDown, int wheelDelta)
{
    // DBG(type << " " << x << " " << y << " " << which
    //          << " " << isShiftDown << " " << wheelDelta);
    auto* e = new MainThreadEventProxy::MouseEvent();
    e->type = String(type);
    e->x = x;
    e->y = y;
    e->which = which;
    e->isShiftDown = isShiftDown;
    e->isCtrlDown = isCtrlDown;
    e->isAltDown = isAltDown;
    e->wheelDelta = wheelDelta;
    MainThreadEventProxy::getInstance().postMessage(e);
}

extern "C" void juce_keyboardCallback(const char* type, int keyCode, const char * key)
{
    // DBG("key " << type << " " << keyCode << " " << key);
    auto* e = new MainThreadEventProxy::KeyboardEvent();
    e->type = String(type);
    e->keyCode = keyCode;
    e->key = String(key);
    MainThreadEventProxy::getInstance().postMessage(e);
}

extern "C" void juce_inputCallback(void* componentPeer,
    const char* type, const char* data)
{
    auto* e = new MainThreadEventProxy::InputEvent();
    e->target = static_cast<EmscriptenComponentPeer*>(componentPeer);
    e->type = String(CharPointer_UTF8(type));
    e->data = String(CharPointer_UTF8(data));
    MainThreadEventProxy::getInstance().postMessage(e);
}

} // namespace juce

//==============================================================================
void launchApp(int argc, char* argv[])
{
    using namespace juce;

    juce_argc = argc;
    juce_argv = argv;

    libraryInitialiser.reset (new ScopedJuceInitialiser_GUI());

    JUCEApplicationBase::createInstance = &juce_CreateApplication;
    JUCEApplicationBase* app = JUCEApplicationBase::createInstance();
    if (! app->initialiseApp())
        exit (app->getApplicationReturnValue());
    
    MessageManager::getInstance();
    MainThreadEventProxy::getInstance();

    jassert (MessageManager::getInstance()->isThisTheMessageThread());
    DBG (SystemStats::getJUCEVersion());

    MessageManager::getInstance()->runDispatchLoop();
}

#include <emscripten.h>
#include <emscripten/threading.h>
#include <unordered_map>

namespace juce
{

extern double getTimeSpentInCurrentDispatchCycle();
extern bool isMessageThreadProxied();

static Point<int> recentMousePosition;
static Array<EmscriptenComponentPeer*> emComponentPeerList;

EM_JS(void, attachEventCallbackToWindow, (),
{
    if (window.juce_mouseCallback) return;

    // event name, x, y, which button, shift, ctrl, alt, wheel delta
    window.juce_mouseCallback = Module.cwrap(
        'juce_mouseCallback', 'void', ['string', 'number', 'number', 'number',
        'number', 'number', 'number', 'number']);
    
    window.onmousedown  = function(e) {
        window.juce_mouseCallback('down' ,
            e.pageX, e.pageY, e.button, e.shiftKey, e.ctrlKey, e.altKey, 0);
    };
    window.onmouseup    = function(e) { 
        window.juce_mouseCallback('up'   ,
            e.pageX, e.pageY, e.button, e.shiftKey, e.ctrlKey, e.altKey, 0);
    };
    window.onmousewheel = function(e) { 
        window.juce_mouseCallback('wheel',
            e.pageX, e.pageY, e.button, e.shiftKey, e.ctrlKey, e.altKey, e.wheelDelta);
    };
    window.onmouseenter = function(e) { 
        window.juce_mouseCallback('enter',
            e.pageX, e.pageY, e.button, e.shiftKey, e.ctrlKey, e.altKey, 0);
    };
    window.onmouseleave = function(e) { 
        window.juce_mouseCallback('leave',
            e.pageX, e.pageY, e.button, e.shiftKey, e.ctrlKey, e.altKey, 0);
    };
    window.onmousemove  = function(e) { 
        window.juce_mouseCallback('move' ,
            e.pageX, e.pageY, e.button, e.shiftKey, e.ctrlKey, e.altKey, 0);
    };

    // window.onmouseout   = function(e) { 
    //     window.juce_mouseCallback('out'  , e.pageX, e.pageY, e.which); };
    // window.onmouseover  = function(e) { 
    //     window.juce_mouseCallback('over' , e.pageX, e.pageY, e.which); };

    // event name, key code, key
    window.juce_keyboardCallback = Module.cwrap(
        'juce_keyboardCallback', 'void', ['string', 'number', 'string']);
    
    // component peer pointer, event name, data
    window.juce_inputCallback = Module.cwrap(
        'juce_inputCallback', 'void', ['number', 'string', 'string']);

    window.addEventListener('keydown', function(e) {
        e.preventDefault();
        window.juce_keyboardCallback ('down', e.which || e.keyCode, e.key);
    });
    window.addEventListener('keyup', function(e) {
        window.juce_keyboardCallback ('up', e.which || e.keyCode, e.key);
    });

    window.juce_clipboard = "";

    window.addEventListener('copy', function(e) {
        navigator.clipboard.readText().then(function(text) {
            window.juce_clipboard = text;
        });
    });
})

class EmscriptenComponentPeer : public ComponentPeer,
                                public MessageListener
{
    Rectangle<int> bounds;
    String id;
    static int highestZIndex;
    int zIndex{0};
    bool focused{false};
    bool visibility{true};
    bool repaintMessagePosted{false};
    double desiredFPS{120.0};

    RectangleList<int> unfinishedRepaintAreas;
    RectangleList<int> pendingRepaintAreas;

    struct RepaintMessage : public Message {};

    public:
        EmscriptenComponentPeer(Component &component_, int styleFlags_)
        :ComponentPeer(component_, styleFlags_)
        {
            emComponentPeerList.add(this);
            DBG("EmscriptenComponentPeer");

            id = Uuid().toDashedString();

            DBG("id is " << id);

            emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_V,
                attachEventCallbackToWindow);

            MAIN_THREAD_EM_ASM_INT({
                var canvas = document.createElement('canvas');
                canvas.id  = UTF8ToString($0);
                canvas.style.zIndex   = $6;
                canvas.style.position = "absolute";
                canvas.style.left = $1;
                canvas.style.top  = $2;
                canvas.width  = $3;
                canvas.height = $4;
                canvas.oncontextmenu = function(e) { e.preventDefault(); };
                canvas.setAttribute('data-peer', $5);
                canvas.addEventListener ('wheel', function(e) {
                    event.preventDefault();
                }, true);
                canvas._duringInput = false;
                canvas._inputProxy = document.createElement('input');
                canvas._inputProxy.type = 'text';
                canvas._inputProxy.style.position = 'absolute';
                canvas._inputProxy.style.opacity = 0;
                canvas._inputProxy.style.zIndex = 0;
                canvas._inputProxy.addEventListener ('compositionstart', function (e)
                {
                    window.juce_inputCallback($5, e.type, e.data);
                });
                canvas._inputProxy.addEventListener ('compositionupdate', function (e)
                {
                    window.juce_inputCallback($5, e.type, e.data);
                });
                canvas._inputProxy.addEventListener ('compositionend', function (e)
                {
                    window.juce_inputCallback($5, e.type, e.data);
                    canvas._inputProxy.value = "";
                });
                canvas._inputProxy.addEventListener ('focus', function (e)
                {
                    if (! canvas._duringInput) canvas.focus();
                });
                canvas._inputProxy.addEventListener ('focusout', function (e)
                {
                    if (canvas._duringInput)
                        canvas._inputProxy.focus();
                });
                document.body.appendChild(canvas);
                document.body.appendChild(canvas._inputProxy);
            }, id.toRawUTF8(), bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), this, ++highestZIndex);
            
            zIndex = highestZIndex;

            grabFocus();

            //if (isFocused())
            //    handleFocusGain();
        }

        ~EmscriptenComponentPeer() override
        {
            emComponentPeerList.removeAllInstancesOf(this);
            MAIN_THREAD_EM_ASM({
                var canvas = document.getElementById(UTF8ToString($0));
                canvas.parentElement.removeChild(canvas._inputProxy);
                canvas.parentElement.removeChild(canvas);
            }, id.toRawUTF8());
        }

        int getZIndex () const { return zIndex; }

        String getId() const { return id; }

        struct ZIndexComparator
        {
            int compareElements (const EmscriptenComponentPeer* first,
                                 const EmscriptenComponentPeer* second) {
                if(first -> getZIndex() < second -> getZIndex()) return -1;
                if(first -> getZIndex() > second -> getZIndex()) return 1;
                return 0;
            }
        };

        virtual void* getNativeHandle () const override
        {
            return (void*)this;
        }

        virtual void setVisible (bool shouldBeVisible) override
        {
            if (visibility == shouldBeVisible) return;

            MAIN_THREAD_EM_ASM({
                var canvas = document.getElementById(UTF8ToString($0));
                canvas.style.visibility = $1 ? "visible" : "hidden";
            }, id.toRawUTF8(), shouldBeVisible);

            visibility = shouldBeVisible;
        }

        bool isVisible () const { return visibility; }

        virtual void setTitle (const String &title) override
        {
            DBG("setTitle: " << title);
            juce::ignoreUnused(title);
        }

        virtual void setBounds (const Rectangle< int > &newBounds, bool isNowFullScreen) override
        {
            DBG("setBounds " << newBounds.toString());

            auto oldBounds = bounds;
            bounds = newBounds;
            fullscreen = isNowFullScreen;

            MAIN_THREAD_EM_ASM({
                var canvas = document.getElementById (UTF8ToString($0));
                canvas.style.left = $1 + 'px';
                canvas.style.top  = $2 + 'px';
                if (canvas.width != $3 || canvas.height != $4)
                {
                    var canvasWasNotEmpty = canvas.width > 0 && canvas.height > 0;
                    if (canvasWasNotEmpty)
                    {
                        var copyCanvas = document.createElement ('canvas');
                        var copyCtx = copyCanvas.getContext ('2d');
                        copyCanvas.width = canvas.width;
                        copyCanvas.height = canvas.height;
                        copyCtx.drawImage (canvas, 0, 0);
                    }
                    canvas.width  = $3;
                    canvas.height = $4;
                    if (canvasWasNotEmpty)
                    {
                        var ctx = canvas.getContext ('2d');
                        ctx.drawImage (copyCanvas, 0, 0);
                    }
                }
            }, id.toRawUTF8(), bounds.getX(), bounds.getY(),
            bounds.getWidth(), bounds.getHeight());
            
            handleMovedOrResized();

            if (! newBounds.isEmpty() &&
                newBounds.withZeroOrigin() != oldBounds.withZeroOrigin())
            {
                repaint(newBounds.withZeroOrigin());
            }
        }

        virtual Rectangle<int> getBounds () const override
        {
            return bounds;
        }

        virtual Point<float>  localToGlobal (Point< float > relativePosition) override
        {
            return relativePosition + bounds.getPosition().toFloat();
        }

        virtual Point<float>  globalToLocal (Point< float > screenPosition) override
        {
            return screenPosition - bounds.getPosition().toFloat();
        }

        virtual void setMinimised (bool) override
        {

        }

        virtual bool isMinimised () const override
        {
            return false;
        }

        bool fullscreen=false;
        virtual void setFullScreen (bool /*shouldBeFullScreen*/) override
        {
            MAIN_THREAD_EM_ASM({
                var canvas = document.getElementById(UTF8ToString($0));
                canvas.style.left='0px';
                canvas.style.top ='0px';

                canvas.width=window.innerWidth;
                canvas.height=window.innerHeight;
            }, id.toRawUTF8());

            bounds = bounds.withZeroOrigin();

            bounds.setWidth ( MAIN_THREAD_EM_ASM_INT({ return window.innerWidth; }, 0) );
            bounds.setHeight( MAIN_THREAD_EM_ASM_INT({ return window.innerHeight; }, 0) );

            this->setBounds(bounds, true);
        }

        virtual bool isFullScreen () const override
        {
            return fullscreen;
        }

        virtual void setIcon (const Image &) override
        {

        }

        virtual bool contains (Point< int > localPos, bool /*trueIfInAChildWindow*/) const override
        {
            Point<int> globalPos = localPos+bounds.getPosition();
            return bounds.contains(globalPos);
        }

        virtual BorderSize<int> getFrameSize () const override
        {
            return BorderSize<int>();
        }

        virtual OptionalBorderSize getFrameSizeIfPresent () const override
        {
            return OptionalBorderSize();
        }

        virtual bool setAlwaysOnTop (bool) override
        {
            return false;
        }

        virtual void toFront (bool makeActive) override
        {
            if (zIndex == highestZIndex) return;
            DBG("toFront " << id << " " << (makeActive ? "true" : "false"));
            
            highestZIndex = MAIN_THREAD_EM_ASM_INT({
                var canvas = document.getElementById(UTF8ToString($0));
                canvas.style.zIndex = parseInt($1)+1;
                return parseInt(canvas.style.zIndex);
            }, id.toRawUTF8(), highestZIndex);

            zIndex = highestZIndex;

            handleBroughtToFront();

            if (makeActive)
            {
                grabFocus();
            }
        }

        void updateZIndex()
        {
            zIndex = MAIN_THREAD_EM_ASM_INT({
                var canvas = document.getElementById(UTF8ToString($0));
                return canvas.zIndex;
            }, id.toRawUTF8());
        }

        virtual void toBehind (ComponentPeer *other) override
        {
            DBG("toBehind");

            if(EmscriptenComponentPeer* otherPeer = dynamic_cast<EmscriptenComponentPeer*>(other))
            {
                int newZIndex = MAIN_THREAD_EM_ASM_INT({
                    var canvas = document.getElementById(UTF8ToString($0));
                    var other  = document.getElementById(UTF8ToString($1));
                    canvas.zIndex = parseInt(other.zIndex)-1;
                    return parseInt(other.zIndex);
                }, id.toRawUTF8(), otherPeer->id.toRawUTF8());

                highestZIndex = std::max(highestZIndex, newZIndex);

                updateZIndex();
                otherPeer->updateZIndex();
                if (! otherPeer->isFocused())
                {
                    otherPeer->focused = true;
                    otherPeer->handleFocusGain();
                }
            }

            if (focused)
            {
                focused = false;
                handleFocusLoss();
            }
        }

        virtual bool isFocused() const override
        {
            return focused;
        }

        virtual void grabFocus() override
        {
            DBG("grabFocus " << id);
            if (! focused)
            {
                for (auto* other : emComponentPeerList)
                {
                    if (other != this && other -> isFocused())
                    {
                        other -> focused = false;
                        other -> handleFocusLoss();
                    }
                }
                focused = true;
                handleFocusGain();
            }
        }

        virtual void textInputRequired(Point< int > position, TextInputTarget &) override
        {
            MAIN_THREAD_EM_ASM({
                var canvas = document.getElementById(UTF8ToString($0));
                var left = parseInt(canvas.style.left, 10);
                var top = parseInt(canvas.style.top, 10);
                canvas._duringInput = true;
                canvas._inputProxy.style.left = (left + $1) + 'px';
                canvas._inputProxy.style.top = (top + $2) + 'px';
                canvas._inputProxy.focus();
            }, id.toRawUTF8(), position.x, position.y);
        }

        virtual void dismissPendingTextInput() override
        {
            MAIN_THREAD_EM_ASM({
                var canvas = document.getElementById(UTF8ToString($0));
                canvas._duringInput = false;
            }, id.toRawUTF8());
        }

        virtual void repaint (const Rectangle<int>& area) override
        {
            pendingRepaintAreas.add(area);
            if (! repaintMessagePosted)
            {
                postMessage (new RepaintMessage());
                repaintMessagePosted = true;
            }
        }

        virtual void performAnyPendingRepaintsNow() override
        {
            DBG("performAnyPendingRepaintsNow");
        }

        virtual void setAlpha (float newAlpha) override
        {
            DBG("setAlpha");
            juce::ignoreUnused(newAlpha);
        }

        virtual StringArray getAvailableRenderingEngines() override
        {
            return StringArray();
        }

    private:

        void handleMessage (const Message& msg) override
        {
            if (dynamic_cast<const RepaintMessage*>(& msg))
            {
                // First finish remaining repaints from the last interrupted
                //   message cycle. This is to prevent a repaint area from being
                //   indefinitely postponed through multiple message cycles.
                for (int i = 0; i < unfinishedRepaintAreas.getNumRectangles(); i ++)
                {
                    Rectangle<int> area = unfinishedRepaintAreas.getRectangle(i);
                    internalRepaint (area);
                    pendingRepaintAreas.subtract (area);
                    if (getTimeSpentInCurrentDispatchCycle() > 1.0 / desiredFPS)
                    {
                        RectangleList<int> remaining;
                        for (int j = i + 1; j < unfinishedRepaintAreas.getNumRectangles(); j ++)
                        {
                            remaining.addWithoutMerging(
                                unfinishedRepaintAreas.getRectangle(j));
                        }
                        unfinishedRepaintAreas = remaining;
                        postMessage (new RepaintMessage());
                        return;
                    }
                }
                
                unfinishedRepaintAreas.clear();

                for (int i = 0; i < pendingRepaintAreas.getNumRectangles(); i ++)
                {
                    Rectangle<int> area = pendingRepaintAreas.getRectangle(i);
                    internalRepaint (area);
                    
                    // Do not interrupt repaints if the message thread is different
                    //   from the main thread since the main loop is no longer blocked
                    //   by the message loop.
                    if (isMessageThreadProxied()) continue;
                    
                    if (getTimeSpentInCurrentDispatchCycle() > 1.0 / desiredFPS)
                    {
                        for (int j = i + 1; j < pendingRepaintAreas.getNumRectangles(); j ++)
                        {
                            unfinishedRepaintAreas.addWithoutMerging(
                                pendingRepaintAreas.getRectangle(j));
                        }
                        pendingRepaintAreas.clear();
                        repaintMessagePosted = true;
                        postMessage (new RepaintMessage());
                        return;
                    }
                }
                repaintMessagePosted = false;
                pendingRepaintAreas.clear();
            }
        }

        void internalRepaint (const Rectangle<int> &area)
        {
            // DBG("repaint: " << area.toString());

            Image temp(Image::ARGB, area.getWidth(), area.getHeight(), true);
            LowLevelGraphicsSoftwareRenderer g(temp);
            g.setOrigin (-area.getPosition());
            handlePaint (g);

            Image::BitmapData bitmapData (temp, Image::BitmapData::readOnly);
            uint8* pixels = bitmapData.getPixelPointer(0, 0);
            int dataSize = bitmapData.width * bitmapData.height * 4;
            for (int i = 0; i < dataSize; i += 4)
                std::swap (pixels[i], pixels[i + 2]);

            MAIN_THREAD_EM_ASM({
                var id = UTF8ToString($0);
                var pointer = $1;
                var width   = $2;
                var height  = $3;

                var canvas = document.getElementById(id);
                var ctx = canvas.getContext("2d");

                var buffer = new Uint8ClampedArray(
                    Module.HEAPU8.buffer, pointer, width * height * 4);
                var imageData = ctx.createImageData(width, height);
                imageData.data.set(buffer);
                ctx.putImageData(imageData, $4, $5);
                delete buffer;
            }, id.toRawUTF8(),
               pixels, bitmapData.width, bitmapData.height,
               area.getX(), area.getY());
        }
};

int EmscriptenComponentPeer::highestZIndex = 10;
static int64 fakeMouseEventTime = 0;
static std::unordered_map<int, bool> keyDownStatus;

void MainThreadEventProxy::handleMouseEvent (const MouseEvent& e)
{
    recentMousePosition = {e.x, e.y};
    bool isDownEvent = e.type == "down";
    bool isUpEvent = e.type == "up";

    ModifierKeys& mods = ModifierKeys::currentModifiers;

    if (isDownEvent)
    {
        mods = mods.withoutMouseButtons();
        if (e.which == 0 || e.which > 2)
            mods = mods.withFlags(ModifierKeys::leftButtonModifier);
        else if (e.which == 1)
            mods = mods.withFlags(ModifierKeys::middleButtonModifier);
        else if (e.which == 2)
            mods = mods.withFlags(ModifierKeys::rightButtonModifier);
    }
    else if (isUpEvent)
    {
        mods = mods.withoutMouseButtons();
    }
    
    mods = e.isShiftDown ? mods.withFlags(ModifierKeys::shiftModifier)
                         : mods.withoutFlags(ModifierKeys::shiftModifier);
    mods = e.isCtrlDown  ? mods.withFlags(ModifierKeys::ctrlModifier)
                         : mods.withoutFlags(ModifierKeys::ctrlModifier);
    mods = e.isAltDown   ? mods.withFlags(ModifierKeys::altModifier)
                         : mods.withoutFlags(ModifierKeys::altModifier);
    
    EmscriptenComponentPeer::ZIndexComparator comparator;
    emComponentPeerList.sort(comparator);
    Point<int> posGlobal(e.x, e.y);

    for (int i = emComponentPeerList.size() - 1; i >= 0; i --)
    {
        EmscriptenComponentPeer* peer = emComponentPeerList[i];

        if (! peer -> isVisible()) continue;

        bool isPosInPeerBounds = peer -> getBounds().contains(posGlobal);
        Point<float> pos = peer->globalToLocal(posGlobal.toFloat());
        if (isDownEvent && ! isPosInPeerBounds) continue;

        if (e.wheelDelta == 0)
        {
            peer->handleMouseEvent(MouseInputSource::InputSourceType::mouse,
                pos, mods, MouseInputSource::defaultPressure, 0.0f, fakeMouseEventTime);
        } else
        {
            MouseWheelDetails wheelInfo;
            wheelInfo.deltaX = 0.0f;
            wheelInfo.deltaY = static_cast<float>(e.wheelDelta) / 480.0f;
            wheelInfo.isReversed = false;
            wheelInfo.isSmooth = false;
            wheelInfo.isInertial = false;
            peer->handleMouseWheel(MouseInputSource::InputSourceType::mouse,
                pos, fakeMouseEventTime, wheelInfo);
        }

        if (isPosInPeerBounds) break; // consume the event
    }
    fakeMouseEventTime ++;
}

void MainThreadEventProxy::handleKeyboardEvent (const KeyboardEvent& e)
{
    bool isChar = e.key.length() == 1;
    bool isDown = e.type == "down";
    juce_wchar keyChar = isChar ? (juce_wchar)e.key[0] : 0;
    int keyCode = e.keyCode;

    ModifierKeys& mods = ModifierKeys::currentModifiers;
    auto changedModifier = ModifierKeys::noModifiers;
    if (keyCode == 16)
        changedModifier = ModifierKeys::shiftModifier;
    else if (keyCode == 17)
        changedModifier = ModifierKeys::ctrlModifier;
    else if (keyCode == 18)
        changedModifier = ModifierKeys::altModifier;
    else if (keyCode == 91)
        changedModifier = ModifierKeys::commandModifier;
    
    if (changedModifier != ModifierKeys::noModifiers)
        mods = isDown ? mods.withFlags(changedModifier)
                      : mods.withoutFlags(changedModifier);
    
    if ((keyChar >= 'a' && keyChar <= 'z') ||
        (keyChar >= 'A' && keyChar <= 'Z'))
        keyCode = keyChar;
    
    keyDownStatus[keyCode] = isDown;

    for (int i = emComponentPeerList.size() - 1; i >= 0; i --)
    {
        EmscriptenComponentPeer* peer = emComponentPeerList[i];

        if (! peer->isVisible()) continue;
        if (! peer->isFocused()) continue;

        if (changedModifier != ModifierKeys::noModifiers)
            peer->handleModifierKeysChange();
        peer->handleKeyUpOrDown(isDown);
        if (isDown)
            peer->handleKeyPress(KeyPress(keyCode, mods, keyChar));
    }
}

void MainThreadEventProxy::handleInputEvent (const InputEvent& e)
{
    TextInputTarget* input = e.target->findCurrentTextInputTarget();
    // DBG("handleInputEvent " << e.type << " " << e.data);
    if (e.type == "compositionstart" || e.type == "compositionupdate")
    {
        Component* inputComponent = dynamic_cast<Component*>(input);
        if (inputComponent)
        {
            auto bounds = inputComponent->getScreenBounds();
            auto caret = input->getCaretRectangle();
            int x = bounds.getX() + caret.getX();
            int y = bounds.getY() + caret.getY();
            MAIN_THREAD_EM_ASM({
                var canvas = document.getElementById(UTF8ToString($0));
                canvas._duringInput = true;
                canvas._inputProxy.style.left = $1 + 'px';
                canvas._inputProxy.style.top = $2 + 'px';
                canvas._inputProxy.focus();
            }, e.target->getId().toRawUTF8(), x, y);
        }
    } else
    if (e.type == "compositionend" && e.data.length() > 0)
        input->insertTextAtCaret (e.data);
}

//==============================================================================
ComponentPeer* Component::createNewPeer (int styleFlags, void*)
{
    return new EmscriptenComponentPeer(*this, styleFlags);
}

//==============================================================================
class Desktop::NativeDarkModeChangeDetectorImpl
{
public:
    NativeDarkModeChangeDetectorImpl() = default;

    virtual ~NativeDarkModeChangeDetectorImpl() = default;

    bool isDarkModeEnabled() const noexcept { return darkModeEnabled; }

private:
    static bool getDarkModeSetting()
    {
        return true;
    }

    bool darkModeEnabled = getDarkModeSetting();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeDarkModeChangeDetectorImpl)
};

std::unique_ptr<Desktop::NativeDarkModeChangeDetectorImpl> Desktop::createNativeDarkModeChangeDetectorImpl()
{
    return std::make_unique<NativeDarkModeChangeDetectorImpl>();
}

bool Desktop::isDarkModeActive() const
{
    return nativeDarkModeChangeDetectorImpl->isDarkModeEnabled();
}


//==============================================================================
bool Desktop::canUseSemiTransparentWindows() noexcept
{
    return true;
}

double Desktop::getDefaultMasterScale()
{
    return 1.0;
}

Desktop::DisplayOrientation Desktop::getCurrentOrientation() const
{
    // TODO
    return upright;
}

bool MouseInputSource::SourceList::addSource()
{
    addSource(sources.size(), MouseInputSource::InputSourceType::mouse);
    return true;
}

bool MouseInputSource::SourceList::canUseTouch()
{
    return false;
}

Point<float> MouseInputSource::getCurrentRawMousePosition()
{
    return recentMousePosition.toFloat();
}

void MouseInputSource::setRawMousePosition (Point<float>)
{
    // not needed
}

//==============================================================================
bool KeyPress::isKeyCurrentlyDown (const int keyCode)
{
    return keyDownStatus[keyCode];
}

//==============================================================================
// TODO
JUCE_API bool JUCE_CALLTYPE Process::isForegroundProcess() { return true; }
JUCE_API void JUCE_CALLTYPE Process::makeForegroundProcess() {}
JUCE_API void JUCE_CALLTYPE Process::hide() {}

//==============================================================================
void Desktop::setScreenSaverEnabled (const bool)
{
    // TODO
}

bool Desktop::isScreenSaverEnabled()
{
    return true;
}

//==============================================================================
void Desktop::setKioskComponent (Component* kioskModeComponent_, bool enableOrDisable, bool allowMenusAndBars)
{
    // TODO
    juce::ignoreUnused(kioskModeComponent_, enableOrDisable, allowMenusAndBars);
}

//==============================================================================
bool juce_areThereAnyAlwaysOnTopWindows()
{
    return false;
}

//==============================================================================
void Displays::findDisplays (float masterScale)
{
    Display d;
    int width = MAIN_THREAD_EM_ASM_INT({
        return document.documentElement.clientWidth;
    },);
    int height = MAIN_THREAD_EM_ASM_INT({
        return document.documentElement.scrollHeight;
    },);
    
    d.totalArea = (Rectangle<float>(static_cast<float>(width), static_cast<float>(height)) / masterScale).toNearestIntEdges();
    d.userArea = d.totalArea;
    d.isMain = true;
    d.scale = masterScale;
    d.dpi = 70;

    displays.add(d);
}

//==============================================================================
Image juce_createIconForFile (const File&)
{
    return Image();
}

//==============================================================================
class MouseCursor::PlatformSpecificHandle
{
public:
    PlatformSpecificHandle(const MouseCursor::StandardCursorType) {}
    PlatformSpecificHandle(const CustomMouseCursorInfo&) {}

    static void showInWindow(PlatformSpecificHandle*, ComponentPeer*) {}
};

//==============================================================================
std::map<enum MouseCursor::StandardCursorType, String> cursorNames = {
    {MouseCursor::StandardCursorType::NoCursor, String("none")},
    {MouseCursor::StandardCursorType::NormalCursor, String("default")},
    {MouseCursor::StandardCursorType::WaitCursor, String("wait")},
    {MouseCursor::StandardCursorType::IBeamCursor, String("text")},
    {MouseCursor::StandardCursorType::CrosshairCursor, String("crosshair")},
    {MouseCursor::StandardCursorType::CopyingCursor, String("copy")},
    {MouseCursor::StandardCursorType::PointingHandCursor, String("pointer")},
    {MouseCursor::StandardCursorType::DraggingHandCursor, String("move")},
    {MouseCursor::StandardCursorType::LeftRightResizeCursor, String("ew-resize")},
    {MouseCursor::StandardCursorType::UpDownResizeCursor, String("ns-resize")},
    {MouseCursor::StandardCursorType::UpDownLeftRightResizeCursor, String("nwse-resize")},
    {MouseCursor::StandardCursorType::TopEdgeResizeCursor, String("n-resize")},
    {MouseCursor::StandardCursorType::BottomEdgeResizeCursor, String("s-resize")},
    {MouseCursor::StandardCursorType::LeftEdgeResizeCursor, String("w-resize")},
    {MouseCursor::StandardCursorType::RightEdgeResizeCursor, String("e-resize")},
    {MouseCursor::StandardCursorType::TopLeftCornerResizeCursor, String("nw-resize")},
    {MouseCursor::StandardCursorType::TopRightCornerResizeCursor, String("ne-resize")},
    {MouseCursor::StandardCursorType::BottomLeftCornerResizeCursor, String("sw-resize")},
    {MouseCursor::StandardCursorType::BottomRightCornerResizeCursor, String("se-resize")}
    };

//==============================================================================
void LookAndFeel::playAlertSound()
{
}

//==============================================================================
void SystemClipboard::copyTextToClipboard (const String& text)
{
    MAIN_THREAD_EM_ASM({
        if (navigator.clipboard)
        {   // async copy
            navigator.clipboard.writeText(UTF8ToString($0));
        } else
        {   // fallback
            var textArea = document.createElement("textarea");
            textArea.value = UTF8ToString($0);
            textArea.style.position = "fixed";
            document.body.appendChild(textArea);
            textArea.focus();
            textArea.select();
            document.execCommand('copy');
            document.body.removeChild(textArea);
        }
    }, text.toRawUTF8());
}

EM_JS(const char*, emscriptenGetClipboard, (), {
    if (window.clipboardUpdater == undefined)
    {
        clipboardUpdater = function(e) {
            navigator.clipboard.readText().then(function(text) {
                window.juce_clipboard = text;
            });
        };
        window.setInterval(clipboardUpdater, 200);
    }
    var data = window.juce_clipboard;
    var dataLen = lengthBytesUTF8(data) + 1;
    var dataOnWASMHeap = _malloc(dataLen);
    stringToUTF8(data, dataOnWASMHeap, dataLen);
    return dataOnWASMHeap;
})

String SystemClipboard::getTextFromClipboard()
{
    const char* data = (const char*)
        emscripten_sync_run_in_main_runtime_thread(
            EM_FUNC_SIG_I, emscriptenGetClipboard);
    String ret = String::fromUTF8(data);
    free((void*)data);
    return ret;
}

void JUCE_CALLTYPE NativeMessageBox::showAsync(const MessageBoxOptions& options,
                                               ModalComponentManager::Callback* callback)
{
    // TODO
    juce::ignoreUnused(options, callback);
}

void JUCE_CALLTYPE NativeMessageBox::showAsync(const MessageBoxOptions& options,
                                               std::function<void(int)> callback)
{
    // TODO
    juce::ignoreUnused(options, callback);
}

// TODO: make async
void JUCE_CALLTYPE NativeMessageBox::showMessageBoxAsync (
    AlertWindow::AlertIconType iconType,
    const String &title,
    const String &message,
    Component *associatedComponent,
    ModalComponentManager::Callback *callback)
{
  juce::ignoreUnused(iconType, title, associatedComponent);

    MAIN_THREAD_EM_ASM({
        alert( UTF8ToString($0) );
    }, message.toRawUTF8());

    if(callback != nullptr) callback->modalStateFinished(1);
}

bool JUCE_CALLTYPE NativeMessageBox::showOkCancelBox(
    AlertWindow::AlertIconType iconType,
    const String &title,
    const String &message,
    Component *associatedComponent,
    ModalComponentManager::Callback *callback)
{
  juce::ignoreUnused(iconType, title, associatedComponent);

    int result = MAIN_THREAD_EM_ASM_INT({
        return window.confirm( UTF8ToString($0) );
    }, message.toRawUTF8());
    if(callback != nullptr) callback->modalStateFinished(result?1:0);

    return static_cast<bool>(result);
}


bool DragAndDropContainer::performExternalDragDropOfFiles (
    const StringArray& files, bool canMoveFiles, Component* sourceComp,
    std::function<void()> callback_)
{
    juce::ignoreUnused(files, canMoveFiles, sourceComp, callback_);
    return false;
}

bool DragAndDropContainer::performExternalDragDropOfText (
    const String& text, Component* sourceComp, std::function<void()> callback_)
{
  juce::ignoreUnused(text, sourceComp, callback_);
    return false;
}

//==============================================================================
const int extendedKeyModifier       = 0x10000;

const int KeyPress::spaceKey        = 32;
const int KeyPress::returnKey       = 13;
const int KeyPress::escapeKey       = 27;
const int KeyPress::backspaceKey    = 8;
const int KeyPress::leftKey         = 37;
const int KeyPress::rightKey        = 39;
const int KeyPress::upKey           = 38;
const int KeyPress::downKey         = 40;
const int KeyPress::pageUpKey       = 33;
const int KeyPress::pageDownKey     = 34;
const int KeyPress::endKey          = 35;
const int KeyPress::homeKey         = 36;
const int KeyPress::deleteKey       = 46;
const int KeyPress::insertKey       = 45;
const int KeyPress::tabKey          = 9;
const int KeyPress::F1Key           = 112;
const int KeyPress::F2Key           = 113;
const int KeyPress::F3Key           = 114;
const int KeyPress::F4Key           = 115;
const int KeyPress::F5Key           = 116;
const int KeyPress::F6Key           = 117;
const int KeyPress::F7Key           = 118;
const int KeyPress::F8Key           = 119;
const int KeyPress::F9Key           = 120;
const int KeyPress::F10Key          = 121;
const int KeyPress::F11Key          = 122;
const int KeyPress::F12Key          = 123;
const int KeyPress::F13Key          = extendedKeyModifier + 24;
const int KeyPress::F14Key          = extendedKeyModifier + 25;
const int KeyPress::F15Key          = extendedKeyModifier + 26;
const int KeyPress::F16Key          = extendedKeyModifier + 27;
const int KeyPress::F17Key          = extendedKeyModifier + 28;
const int KeyPress::F18Key          = extendedKeyModifier + 29;
const int KeyPress::F19Key          = extendedKeyModifier + 30;
const int KeyPress::F20Key          = extendedKeyModifier + 31;
const int KeyPress::F21Key          = extendedKeyModifier + 32;
const int KeyPress::F22Key          = extendedKeyModifier + 33;
const int KeyPress::F23Key          = extendedKeyModifier + 34;
const int KeyPress::F24Key          = extendedKeyModifier + 35;
const int KeyPress::F25Key          = extendedKeyModifier + 36;
const int KeyPress::F26Key          = extendedKeyModifier + 37;
const int KeyPress::F27Key          = extendedKeyModifier + 38;
const int KeyPress::F28Key          = extendedKeyModifier + 39;
const int KeyPress::F29Key          = extendedKeyModifier + 40;
const int KeyPress::F30Key          = extendedKeyModifier + 41;
const int KeyPress::F31Key          = extendedKeyModifier + 42;
const int KeyPress::F32Key          = extendedKeyModifier + 43;
const int KeyPress::F33Key          = extendedKeyModifier + 44;
const int KeyPress::F34Key          = extendedKeyModifier + 45;
const int KeyPress::F35Key          = extendedKeyModifier + 46;
const int KeyPress::numberPad0      = extendedKeyModifier + 27;
const int KeyPress::numberPad1      = extendedKeyModifier + 28;
const int KeyPress::numberPad2      = extendedKeyModifier + 29;
const int KeyPress::numberPad3      = extendedKeyModifier + 30;
const int KeyPress::numberPad4      = extendedKeyModifier + 31;
const int KeyPress::numberPad5      = extendedKeyModifier + 32;
const int KeyPress::numberPad6      = extendedKeyModifier + 33;
const int KeyPress::numberPad7      = extendedKeyModifier + 34;
const int KeyPress::numberPad8      = extendedKeyModifier + 35;
const int KeyPress::numberPad9      = extendedKeyModifier + 36;
const int KeyPress::numberPadAdd            = extendedKeyModifier + 37;
const int KeyPress::numberPadSubtract       = extendedKeyModifier + 38;
const int KeyPress::numberPadMultiply       = extendedKeyModifier + 39;
const int KeyPress::numberPadDivide         = extendedKeyModifier + 40;
const int KeyPress::numberPadSeparator      = extendedKeyModifier + 41;
const int KeyPress::numberPadDecimalPoint   = extendedKeyModifier + 42;
const int KeyPress::numberPadEquals         = extendedKeyModifier + 43;
const int KeyPress::numberPadDelete         = extendedKeyModifier + 44;
const int KeyPress::playKey         = extendedKeyModifier + 45;
const int KeyPress::stopKey         = extendedKeyModifier + 46;
const int KeyPress::fastForwardKey  = extendedKeyModifier + 47;
const int KeyPress::rewindKey       = extendedKeyModifier + 48;

}

#pragma clang diagnostic pop
