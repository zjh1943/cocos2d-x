/****************************************************************************
Copyright (c) 2013-2014 Chukong Technologies Inc.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "ui/UIWidget.h"
#include "ui/UILayout.h"
#include "ui/UIHelper.h"

NS_CC_BEGIN

namespace ui {

Widget* Widget::_focusedWidget = nullptr;
Widget* Widget::_realFocusedWidget = nullptr;
    
Widget::Widget():
_enabled(true),
_bright(true),
_touchEnabled(false),
_touchPassedEnabled(false),
_highlight(false),
_brightStyle(BrightStyle::NONE),
_touchStartPos(Vector2::ZERO),
_touchMovePos(Vector2::ZERO),
_touchEndPos(Vector2::ZERO),
_touchEventListener(nullptr),
_touchEventSelector(nullptr),
_name("default"),
_actionTag(0),
_size(Size::ZERO),
_customSize(Size::ZERO),
_ignoreSize(false),
_affectByClipping(false),
_sizeType(SizeType::ABSOLUTE),
_sizePercent(Vector2::ZERO),
_positionType(PositionType::ABSOLUTE),
_positionPercent(Vector2::ZERO),
_reorderWidgetChildDirty(true),
_hitted(false),
_touchListener(nullptr),
_color(Color3B::WHITE),
_opacity(255),
_flippedX(false),
_flippedY(false),
_focused(false),
_focusEnabled(true)
{
    onFocusChanged = CC_CALLBACK_2(Widget::onFocusChange,this);
    onNextFocusedWidget = nullptr;
    this->setAnchorPoint(Vector2(0.5f, 0.5f));
}

Widget::~Widget()
{
    setTouchEnabled(false);
    
    //cleanup focused widget
    if (_focusedWidget == this) {
        _focusedWidget = nullptr;
    }
    if (_realFocusedWidget == this) {
        _realFocusedWidget = nullptr;
    }
}

Widget* Widget::create()
{
    Widget* widget = new Widget();
    if (widget && widget->init())
    {
        widget->autorelease();
        return widget;
    }
    CC_SAFE_DELETE(widget);
    return nullptr;
}

bool Widget::init()
{
    if (ProtectedNode::init())
    {
        initRenderer();
        setBright(true);
        ignoreContentAdaptWithSize(true);
        setAnchorPoint(Vector2(0.5f, 0.5f));
        return true;
    }
    return false;
}

void Widget::onEnter()
{
    updateSizeAndPosition();
    ProtectedNode::onEnter();
}

void Widget::onExit()
{
    unscheduleUpdate();
    ProtectedNode::onExit();
}

void Widget::visit(Renderer *renderer, const Matrix &parentTransform, bool parentTransformUpdated)
{
    if (_enabled)
    {
        adaptRenderers();
        ProtectedNode::visit(renderer, parentTransform, parentTransformUpdated);
    }
}

Widget* Widget::getWidgetParent()
{
    return dynamic_cast<Widget*>(getParent());
}

void Widget::setEnabled(bool enabled)
{
    _enabled = enabled;
    for (auto& child : _children)
    {
        if (child)
        {
            Widget* widgetChild = dynamic_cast<Widget*>(child);
            if (widgetChild)
            {
                widgetChild->setEnabled(enabled);
            }
        }
    }
    
    for (auto& child : _protectedChildren)
    {
        if (child)
        {
            Widget* widgetChild = dynamic_cast<Widget*>(child);
            if (widgetChild)
            {
                widgetChild->setEnabled(enabled);
            }
        }
    }
}

Widget* Widget::getChildByName(const std::string& name)
{
    for (auto& child : _children)
    {
        if (child)
        {
            Widget* widgetChild = dynamic_cast<Widget*>(child);
            if (widgetChild)
            {
                if (widgetChild->getName() == name)
                {
                    return widgetChild;
                }
            }
        }
    }
    return nullptr;
}
    
void Widget::initRenderer()
{
}

void Widget::setSize(const Size &size)
{
    _customSize = size;
    if (_ignoreSize)
    {
        _size = getVirtualRendererSize();
    }
    else
    {
        _size = size;
    }
    if (_running)
    {
        Widget* widgetParent = getWidgetParent();
        Size pSize;
        if (widgetParent)
        {
            pSize = widgetParent->getSize();
        }
        else
        {
            pSize = _parent->getContentSize();
        }
        float spx = 0.0f;
        float spy = 0.0f;
        if (pSize.width > 0.0f)
        {
            spx = _customSize.width / pSize.width;
        }
        if (pSize.height > 0.0f)
        {
            spy = _customSize.height / pSize.height;
        }
        _sizePercent = Vector2(spx, spy);
    }
    onSizeChanged();
}

void Widget::setSizePercent(const Vector2 &percent)
{
    _sizePercent = percent;
    Size cSize = _customSize;
    if (_running)
    {
        Widget* widgetParent = getWidgetParent();
        if (widgetParent)
        {
            cSize = Size(widgetParent->getSize().width * percent.x , widgetParent->getSize().height * percent.y);
        }
        else
        {
            cSize = Size(_parent->getContentSize().width * percent.x , _parent->getContentSize().height * percent.y);
        }
    }
    if (_ignoreSize)
    {
        _size = getVirtualRendererSize();
    }
    else
    {
        _size = cSize;
    }
    _customSize = cSize;
    onSizeChanged();
}

void Widget::updateSizeAndPosition()
{
    Widget* widgetParent = getWidgetParent();
    Size pSize;
    if (widgetParent)
    {
        pSize = widgetParent->getLayoutSize();
    }
    else
    {
        pSize = _parent->getContentSize();
    }
    updateSizeAndPosition(pSize);
}
    
void Widget::updateSizeAndPosition(const cocos2d::Size &parentSize)
{
    switch (_sizeType)
    {
        case SizeType::ABSOLUTE:
        {
            if (_ignoreSize)
            {
                _size = getVirtualRendererSize();
            }
            else
            {
                _size = _customSize;
            }
            float spx = 0.0f;
            float spy = 0.0f;
            if (parentSize.width > 0.0f)
            {
                spx = _customSize.width / parentSize.width;
            }
            if (parentSize.height > 0.0f)
            {
                spy = _customSize.height / parentSize.height;
            }
            _sizePercent = Vector2(spx, spy);
            break;
        }
        case SizeType::PERCENT:
        {
            Size cSize = Size(parentSize.width * _sizePercent.x , parentSize.height * _sizePercent.y);
            if (_ignoreSize)
            {
                _size = getVirtualRendererSize();
            }
            else
            {
                _size = cSize;
            }
            _customSize = cSize;
            break;
        }
        default:
            break;
    }
    onSizeChanged();
    Vector2 absPos = getPosition();
    switch (_positionType)
    {
        case PositionType::ABSOLUTE:
        {
            if (parentSize.width <= 0.0f || parentSize.height <= 0.0f)
            {
                _positionPercent = Vector2::ZERO;
            }
            else
            {
                _positionPercent = Vector2(absPos.x / parentSize.width, absPos.y / parentSize.height);
            }
            break;
        }
        case PositionType::PERCENT:
        {
            absPos = Vector2(parentSize.width * _positionPercent.x, parentSize.height * _positionPercent.y);
            break;
        }
        default:
            break;
    }
    setPosition(absPos);
}

void Widget::setSizeType(SizeType type)
{
    _sizeType = type;
}

Widget::SizeType Widget::getSizeType() const
{
    return _sizeType;
}

void Widget::ignoreContentAdaptWithSize(bool ignore)
{
    if (_ignoreSize == ignore)
    {
        return;
    }
    _ignoreSize = ignore;
    if (_ignoreSize)
    {
        Size s = getVirtualRendererSize();
        _size = s;
    }
    else
    {
        _size = _customSize;
    }
    onSizeChanged();
}

bool Widget::isIgnoreContentAdaptWithSize() const
{
    return _ignoreSize;
}

const Size& Widget::getSize() const
{
    return _size;
}
    
const Size& Widget::getCustomSize() const
{
    return _customSize;
}

const Vector2& Widget::getSizePercent() const
{
    return _sizePercent;
}

Vector2 Widget::getWorldPosition()
{
    return convertToWorldSpace(Vector2(_anchorPoint.x * _contentSize.width, _anchorPoint.y * _contentSize.height));
}

Node* Widget::getVirtualRenderer()
{
    return this;
}

void Widget::onSizeChanged()
{
    setContentSize(_size);
    for (auto& child : getChildren())
    {
        Widget* widgetChild = dynamic_cast<Widget*>(child);
        if (widgetChild)
        {
            widgetChild->updateSizeAndPosition();
        }
    }
}

const Size& Widget::getVirtualRendererSize() const
{
    return _contentSize;
}
    
void Widget::updateContentSizeWithTextureSize(const cocos2d::Size &size)
{
    if (_ignoreSize)
    {
        _size = size;
    }
    else
    {
        _size = _customSize;
    }
    onSizeChanged();
}

void Widget::setTouchEnabled(bool enable)
{
    if (enable == _touchEnabled)
    {
        return;
    }
    _touchEnabled = enable;
    if (_touchEnabled)
    {
        _touchListener = EventListenerTouchOneByOne::create();
        CC_SAFE_RETAIN(_touchListener);
        _touchListener->setSwallowTouches(true);
        _touchListener->onTouchBegan = CC_CALLBACK_2(Widget::onTouchBegan, this);
        _touchListener->onTouchMoved = CC_CALLBACK_2(Widget::onTouchMoved, this);
        _touchListener->onTouchEnded = CC_CALLBACK_2(Widget::onTouchEnded, this);
        _touchListener->onTouchCancelled = CC_CALLBACK_2(Widget::onTouchCancelled, this);
        _eventDispatcher->addEventListenerWithSceneGraphPriority(_touchListener, this);
    }
    else
    {
        _eventDispatcher->removeEventListener(_touchListener);
        CC_SAFE_RELEASE_NULL(_touchListener);
    }
}

bool Widget::isTouchEnabled() const
{
    return _touchEnabled;
}

bool Widget::isHighlighted() const
{
    return _highlight;
}

void Widget::setHighlighted(bool hilight)
{
    if (hilight == _highlight)
    {
        return;
    }
    _highlight = hilight;
    if (_bright)
    {
        if (_highlight)
        {
            setBrightStyle(BrightStyle::HIGHLIGHT);
        }
        else
        {
            setBrightStyle(BrightStyle::NORMAL);
        }
    }
    else
    {
        onPressStateChangedToDisabled();
    }
}

void Widget::setBright(bool bright)
{
    _bright = bright;
    if (_bright)
    {
        _brightStyle = BrightStyle::NONE;
        setBrightStyle(BrightStyle::NORMAL);
    }
    else
    {
        onPressStateChangedToDisabled();
    }
}

void Widget::setBrightStyle(BrightStyle style)
{
    if (_brightStyle == style)
    {
        return;
    }
    _brightStyle = style;
    switch (_brightStyle)
    {
        case BrightStyle::NORMAL:
            onPressStateChangedToNormal();
            break;
        case BrightStyle::HIGHLIGHT:
            onPressStateChangedToPressed();
            break;
        default:
            break;
    }
}

void Widget::onPressStateChangedToNormal()
{

}

void Widget::onPressStateChangedToPressed()
{

}

void Widget::onPressStateChangedToDisabled()
{

}

void Widget::didNotSelectSelf()
{

}

bool Widget::onTouchBegan(Touch *touch, Event *unusedEvent)
{
    _hitted = false;
    if (isEnabled() && isTouchEnabled())
    {
        _touchStartPos = touch->getLocation();
        if(hitTest(_touchStartPos) && clippingParentAreaContainPoint(_touchStartPos))
        {
            _hitted = true;
        }
    }
    if (!_hitted)
    {
        return false;
    }
    setHighlighted(true);
    Widget* widgetParent = getWidgetParent();
    if (widgetParent)
    {
        widgetParent->checkChildInfo(0,this,_touchStartPos);
    }
    pushDownEvent();
    return !_touchPassedEnabled;
}

void Widget::onTouchMoved(Touch *touch, Event *unusedEvent)
{
    _touchMovePos = touch->getLocation();
    setHighlighted(hitTest(_touchMovePos));
    Widget* widgetParent = getWidgetParent();
    if (widgetParent)
    {
        widgetParent->checkChildInfo(1,this,_touchMovePos);
    }
    moveEvent();
}

void Widget::onTouchEnded(Touch *touch, Event *unusedEvent)
{
    _touchEndPos = touch->getLocation();
    bool highlight = _highlight;
    setHighlighted(false);
    Widget* widgetParent = getWidgetParent();
    if (widgetParent)
    {
        widgetParent->checkChildInfo(2,this,_touchEndPos);
    }
    if (highlight)
    {
        releaseUpEvent();
    }
    else
    {
        cancelUpEvent();
    }
}

void Widget::onTouchCancelled(Touch *touch, Event *unusedEvent)
{
    setHighlighted(false);
    cancelUpEvent();
}

void Widget::pushDownEvent()
{
    if (_touchEventCallback) {
        _touchEventCallback(this, TouchEventType::BEGAN);
    }
    
    if (_touchEventListener && _touchEventSelector)
    {
        (_touchEventListener->*_touchEventSelector)(this,TOUCH_EVENT_BEGAN);
    }
}

void Widget::moveEvent()
{
    if (_touchEventCallback) {
        _touchEventCallback(this, TouchEventType::MOVED);
    }
    
    if (_touchEventListener && _touchEventSelector)
    {
        (_touchEventListener->*_touchEventSelector)(this,TOUCH_EVENT_MOVED);
    }
}

void Widget::releaseUpEvent()
{
    
    if (_touchEventCallback) {
        _touchEventCallback(this, TouchEventType::ENDED);
    }
    
    if (_touchEventListener && _touchEventSelector)
    {
        (_touchEventListener->*_touchEventSelector)(this,TOUCH_EVENT_ENDED);
    }
}

void Widget::cancelUpEvent()
{
    if (_touchEventCallback)
    {
        _touchEventCallback(this, TouchEventType::CANCELED);
    }
   
    if (_touchEventListener && _touchEventSelector)
    {
        (_touchEventListener->*_touchEventSelector)(this,TOUCH_EVENT_CANCELED);
    }
   
}

void Widget::addTouchEventListener(Ref *target, SEL_TouchEvent selector)
{
    _touchEventListener = target;
    _touchEventSelector = selector;
}
    
void Widget::addTouchEventListener(Widget::ccWidgetTouchCallback callback)
{
    this->_touchEventCallback = callback;
}

bool Widget::hitTest(const Vector2 &pt)
{
    Vector2 nsp = convertToNodeSpace(pt);
    Rect bb;
    bb.size = _contentSize;
    if (bb.containsPoint(nsp))
    {
        return true;
    }
    return false;
}

bool Widget::clippingParentAreaContainPoint(const Vector2 &pt)
{
    _affectByClipping = false;
    Widget* parent = getWidgetParent();
    Widget* clippingParent = nullptr;
    while (parent)
    {
        Layout* layoutParent = dynamic_cast<Layout*>(parent);
        if (layoutParent)
        {
            if (layoutParent->isClippingEnabled())
            {
                _affectByClipping = true;
                clippingParent = layoutParent;
                break;
            }
        }
        parent = parent->getWidgetParent();
    }

    if (!_affectByClipping)
    {
        return true;
    }


    if (clippingParent)
    {
        bool bRet = false;
        if (clippingParent->hitTest(pt))
        {
            bRet = true;
        }
        if (bRet)
        {
            return clippingParent->clippingParentAreaContainPoint(pt);
        }
        return false;
    }
    return true;
}

void Widget::checkChildInfo(int handleState, Widget *sender, const Vector2 &touchPoint)
{
    Widget* widgetParent = getWidgetParent();
    if (widgetParent)
    {
        widgetParent->checkChildInfo(handleState,sender,touchPoint);
    }
}

void Widget::setPosition(const Vector2 &pos)
{
    if (_running)
    {
        Widget* widgetParent = getWidgetParent();
        if (widgetParent)
        {
            Size pSize = widgetParent->getSize();
            if (pSize.width <= 0.0f || pSize.height <= 0.0f)
            {
                _positionPercent = Vector2::ZERO;
            }
            else
            {
                _positionPercent = Vector2(pos.x / pSize.width, pos.y / pSize.height);
            }
        }
    }
    ProtectedNode::setPosition(pos);
}

void Widget::setPositionPercent(const Vector2 &percent)
{
    _positionPercent = percent;
    if (_running)
    {
        Widget* widgetParent = getWidgetParent();
        if (widgetParent)
        {
            Size parentSize = widgetParent->getSize();
            Vector2 absPos = Vector2(parentSize.width * _positionPercent.x, parentSize.height * _positionPercent.y);
            setPosition(absPos);
        }
    }
}

const Vector2& Widget::getPositionPercent()
{
    return _positionPercent;
}

void Widget::setPositionType(PositionType type)
{
    _positionType = type;
}

Widget::PositionType Widget::getPositionType() const
{
    return _positionType;
}

bool Widget::isBright() const
{
    return _bright;
}

bool Widget::isEnabled() const
{
    return _enabled;
}

float Widget::getLeftInParent()
{
    return getPosition().x - getAnchorPoint().x * _size.width;;
}

float Widget::getBottomInParent()
{
    return getPosition().y - getAnchorPoint().y * _size.height;;
}

float Widget::getRightInParent()
{
    return getLeftInParent() + _size.width;
}

float Widget::getTopInParent()
{
    return getBottomInParent() + _size.height;
}

const Vector2& Widget::getTouchStartPos()
{
    return _touchStartPos;
}

const Vector2& Widget::getTouchMovePos()
{
    return _touchMovePos;
}

const Vector2& Widget::getTouchEndPos()
{
    return _touchEndPos;
}

void Widget::setName(const std::string& name)
{
    _name = name;
}

const std::string& Widget::getName() const
{
    return _name;
}


void Widget::setLayoutParameter(LayoutParameter *parameter)
{
    if (!parameter)
    {
        return;
    }
    _layoutParameterDictionary.insert((int)parameter->getLayoutType(), parameter);
}

LayoutParameter* Widget::getLayoutParameter(LayoutParameter::Type type)
{
    return dynamic_cast<LayoutParameter*>(_layoutParameterDictionary.at((int)type));
}

std::string Widget::getDescription() const
{
    return "Widget";
}

Widget* Widget::clone()
{
    Widget* clonedWidget = createCloneInstance();
    clonedWidget->copyProperties(this);
    clonedWidget->copyClonedWidgetChildren(this);
    return clonedWidget;
}

Widget* Widget::createCloneInstance()
{
    return Widget::create();
}

void Widget::copyClonedWidgetChildren(Widget* model)
{
    auto& modelChildren = model->getChildren();

    for (auto& subWidget : modelChildren)
    {
        Widget* child = dynamic_cast<Widget*>(subWidget);
        if (child)
        {
            addChild(child->clone());
        }
    }
}

void Widget::copySpecialProperties(Widget* model)
{

}

void Widget::copyProperties(Widget *widget)
{
    setEnabled(widget->isEnabled());
    setVisible(widget->isVisible());
    setBright(widget->isBright());
    setTouchEnabled(widget->isTouchEnabled());
    _touchPassedEnabled = false;
    setLocalZOrder(widget->getLocalZOrder());
    setTag(widget->getTag());
    setName(widget->getName());
    setActionTag(widget->getActionTag());
    _ignoreSize = widget->_ignoreSize;
    _size = widget->_size;
    _customSize = widget->_customSize;
    copySpecialProperties(widget);
    _sizeType = widget->getSizeType();
    _sizePercent = widget->_sizePercent;
    _positionType = widget->_positionType;
    _positionPercent = widget->_positionPercent;
    setPosition(widget->getPosition());
    setAnchorPoint(widget->getAnchorPoint());
    setScaleX(widget->getScaleX());
    setScaleY(widget->getScaleY());
    setRotation(widget->getRotation());
    setRotationSkewX(widget->getRotationSkewX());
    setRotationSkewY(widget->getRotationSkewY());
    setFlippedX(widget->isFlippedX());
    setFlippedY(widget->isFlippedY());
    setColor(widget->getColor());
    setOpacity(widget->getOpacity());
    //FIXME:copy focus properties, also make sure all the subclass the copy behavior is correct
    Map<int, LayoutParameter*>& layoutParameterDic = widget->_layoutParameterDictionary;
    for (auto iter = layoutParameterDic.begin(); iter != layoutParameterDic.end(); ++iter)
    {
        setLayoutParameter(iter->second->clone());
    }
    onSizeChanged();
}
    
void Widget::setColor(const Color3B& color)
{
    _color = color;
    updateTextureColor();
}

void Widget::setOpacity(GLubyte opacity)
{
    _opacity = opacity;
    updateTextureOpacity();
}
    
void Widget::setFlippedX(bool flippedX)
{
    _flippedX = flippedX;
    updateFlippedX();
}

void Widget::setFlippedY(bool flippedY)
{
    _flippedY = flippedY;
    updateFlippedY();
}

void Widget::updateColorToRenderer(Node* renderer)
{
    renderer->setColor(_color);
}

void Widget::updateOpacityToRenderer(Node* renderer)
{
    renderer->setOpacity(_opacity);
}

void Widget::updateRGBAToRenderer(Node* renderer)
{
    renderer->setColor(_color);
    renderer->setOpacity(_opacity);
}

/*temp action*/
void Widget::setActionTag(int tag)
{
	_actionTag = tag;
}

int Widget::getActionTag()
{
	return _actionTag;
}
    
void Widget::setFocused(bool focus)
{
    _focused = focus;
    
    //make sure there is only one focusedWidget
    if (focus) {
        _focusedWidget = this;
        if (!dynamic_cast<Layout*>(this)) {
            _realFocusedWidget = this;
        }
    }
    
}

bool Widget::isFocused()
{
    return _focused;
}

void Widget::setFocusEnabled(bool enable)
{
    _focusEnabled = enable;
}

bool Widget::isFocusEnabled()
{
    return _focusEnabled;
}

Widget* Widget::findNextFocusedWidget(FocusDirection direction,  Widget* current)
{
    if (nullptr == onNextFocusedWidget || nullptr == onNextFocusedWidget(direction) ) {
        if (this->isFocused() || !current->isFocusEnabled())
        {
            Node* parent = this->getParent();
            
            Layout* layout = dynamic_cast<Layout*>(parent);
            if (nullptr == layout)
            {
                //the outer layout's default behaviour is : loop focus
                if (dynamic_cast<Layout*>(current))
                {
                    return current->findNextFocusedWidget(direction, current);
                }
                return current;
            }
            else
            {
                Widget *nextWidget = layout->findNextFocusedWidget(direction, current);
                return nextWidget;
            }
        }
        else
        {
            return current;
        }
    }
    else
    {
        Widget *getFocusWidget = onNextFocusedWidget(direction);
        this->dispatchFocusEvent(this, getFocusWidget);
        return getFocusWidget;
    }
}

void Widget::dispatchFocusEvent(cocos2d::ui::Widget *widgetLoseFocus, cocos2d::ui::Widget *widgetGetFocus)
{
    //if the widgetLoseFocus doesn't get focus, it will use the previous focused widget instead
    if (widgetLoseFocus && !widgetLoseFocus->isFocused())
    {
        widgetLoseFocus = _focusedWidget;
    }
    
    if (widgetGetFocus != widgetLoseFocus)
    {
        
        if (widgetGetFocus)
        {
            widgetGetFocus->onFocusChanged(widgetLoseFocus, widgetGetFocus);
        }
        
        if (widgetLoseFocus)
        {
            widgetLoseFocus->onFocusChanged(widgetLoseFocus, widgetGetFocus);
        }
        
        EventFocus event(widgetLoseFocus, widgetGetFocus);
        auto dispatcher = cocos2d::Director::getInstance()->getEventDispatcher();
        dispatcher->dispatchEvent(&event);
    }
    
}

void Widget::requestFocus()
{
    if (this == _focusedWidget)
    {
        return;
    }
    
    this->dispatchFocusEvent(_focusedWidget, this);
}
    
void Widget::onFocusChange(Widget* widgetLostFocus, Widget* widgetGetFocus)
{
    //only change focus when there is indeed a get&lose happens
    if (widgetLostFocus)
    {
        widgetLostFocus->setFocused(false);
    }
    
    if (widgetGetFocus)
    {
        widgetGetFocus->setFocused(true);
    }
}

Widget* Widget::getCurrentFocusedWidget(bool isWidget)
{
    if (isWidget) {
        return _realFocusedWidget;
    }
    return _focusedWidget;
}



}

NS_CC_END
