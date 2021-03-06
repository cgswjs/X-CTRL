#include "FileGroup.h"
#include "DisplayPrivate.h"
#include "ComPrivate.h"

/*实例化当前页面调度器*/
static MillisTaskManager mtm_MainMenu(4);

/*贴图位置设置*/
#define PosCtrlInfo 0.25f
#define PosOptions  0.75f
#define PosCursorY  0.8f

/*实例化光标控件*/
static LightGUI::Cursor<SCREEN_CLASS> MenuCursor(&screen, PosCursorY * screen.height(), 100, 60, 2);

#define CursorCloseTo(pos) (ABS(CursorPosNow - pos)<0.01f)

static float CursorPosNow = PosCtrlInfo;
static float CursorPosLast = PosCtrlInfo;
static uint8_t CursorSelect = 0;
static uint8_t CursorSelect_Stop = 0;
static const float CursorTargetNow[] = {PosCtrlInfo, PosOptions};
static uint32_t StayPageTimePoint;
static bool IsSpoke;

/**
  * @brief  光标位置更新任务
  * @param  无
  * @retval 无
  */
static void Task_CursorUpdate()
{
    /*更新位置*/
    CursorPosNow = CursorTargetNow[CursorSelect % __Sizeof(CursorTargetNow)];

    if(!CursorCloseTo(CursorPosLast))
        CursorPosLast += (CursorPosNow - CursorPosLast) * 0.3f;

    MenuCursor.setPosition_f(CursorPosLast, PosCursorY); //设置控件位置
}

static void Task_UpdateTextOpacity()
{
    static const uint8_t OpacityStep = 40;
    static uint8_t Opacity = 0;
    if(CursorSelect_Stop == 0)
    {
        if(Opacity < 0xff - OpacityStep)
        {
            Opacity += OpacityStep;
        }
        else
        {
            Opacity = 0xff;
        }
    }
    else if(CursorSelect_Stop == 1)
    {
        if(Opacity > OpacityStep)
        {
            Opacity -= OpacityStep;
        }
        else
        {
            Opacity = 0;
        }
    }

    screen.setTextColor(screen.White, screen.Black);
    screen.setCursor(17, StatusBar_Height + 15);
    screen.setOpacityX(Opacity);
    screen.printfX("PLAY");

    screen.setCursor(76, StatusBar_Height + 15);
    screen.setOpacityX(255 - Opacity);
    screen.printfX("CONFIG");

    screen.setOpacityX(255);
}

static void Task_PlayVideoFast_1()
{
    player1.UpdatePlayerState();

    if(CursorCloseTo(PosCtrlInfo))
    {
//        player1.UpdatePlayerState();
        CursorSelect_Stop = 0;
    }
    else if(CursorCloseTo(PosOptions))
    {
//        player2.UpdatePlayerState();
        CursorSelect_Stop = 1;
    }
}

static void Task_PlayVideoFast_2()
{
    player2.UpdatePlayerState();
}

/**
  * @brief  页面初始化事件
  * @param  无
  * @retval 无
  */
static void Setup()
{
    ClearPage();
    StayPageTimePoint = millis();//记录页面进入时间戳

    IsSpoke = false;

    /*光标弹出动画*/
    for(float y = 1.1f; y > PosCursorY; y -= 0.01f)
    {
        MenuCursor.setPosition_f(CursorPosLast, y);
        PageDelay(10);
    }

    player1.UpdatePlayerState();
    player2.UpdatePlayerState();
    /*任务注册*/
    mtm_MainMenu.TaskRegister(0, Task_CursorUpdate, 30);
    mtm_MainMenu.TaskRegister(1, Task_PlayVideoFast_1, 50);
    mtm_MainMenu.TaskRegister(2, Task_PlayVideoFast_2, 50);
    mtm_MainMenu.TaskRegister(3, Task_UpdateTextOpacity, 50, OFF);
}

/**
  * @brief  页面循环事件
  * @param  无
  * @retval 无
  */
static void Loop()
{
    if(!IsSpoke && millis() - StayPageTimePoint > 10000)
    {
        if(CTRL.Battery.Usage < 30)
            XFS_Speak("警告：电池电量低。");

        XFS_Speak(
            "当前电池电压%0.1f伏，剩余电量%0.0f%%",
            CTRL.Battery.Voltage,
            CTRL.Battery.Usage
        );

        if(CTRL.Battery.Usage < 20)
            XFS_Speak("请立即充电");
        else if(CTRL.Battery.Usage < 30)
            XFS_Speak("请及时充电");

        IsSpoke = true;
    }
    mtm_MainMenu.Running(millis());
}

/**
  * @brief  页面退出事件
  * @param  无
  * @retval 无
  */
static void Exit()
{
    /*光标下落动画*/
    for(float y = PosCursorY; y < 1.1f; y += 0.01f)
    {
        MenuCursor.setPosition_f(CursorPosLast, y);
        PageDelay(10);
    }
}

/**
  * @brief  页面事件
  * @param  无
  * @retval 无
  */
static void Event(int event, void* param)
{
    if(event == EVENT_ButtonPress)
    {
        if(btDOWN && CursorSelect > 0)CursorSelect--;
        if(btUP && CursorSelect < 1)CursorSelect++;

        if(btEcd || btOK)
        {
            if(CursorCloseTo(PosCtrlInfo))
            {
                CursorPosLast = PosCtrlInfo;
                if(CTRL.State.Handshake)
                    page.PagePush(PAGE_Handshake);
                else
                    page.PagePush(PAGE_CtrlInfo);
            }
            else if(CursorCloseTo(PosOptions))
            {
                CursorPosLast = PosOptions;
                page.PagePush(PAGE_Options);
            }
        }
    }
}

/**
  * @brief  主菜单页面注册
  * @param  pageID:为此页面分配的ID号
  * @retval 无
  */
void PageRegister_MainMenuDymanic(uint8_t pageID)
{
    page.PageRegister(pageID, Setup, Loop, Exit, Event);
}
