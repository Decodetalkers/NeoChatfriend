#include "MainWindow.hpp"

MainWindow::MainWindow (QWidget* parent)
  : QMainWindow (parent), ui (new Ui_MainWindow), user_menu (new UserMenuWidget),
    userSetting (new QSettings ("ChatGpt.ini", QSettings::NativeFormat)),
    login_widget (new LoginWidget) {
  ui->setupUi (this);
  user_menu->setParent (this);
  login_widget->setParent (this);
  ui->SideMenuButton->installEventFilter (this);
  resize (1300, 900);
  installEventFilter (this);
  setWindowTitle ("Chat Client");
  setWindowFlags (Qt::Tool);
  new_user_account = GetLastLoginAccount();
  user[new_user_account] = User (*userSetting, new_user_account);
  InitNetWork();
  ui->statusbar->setStyleSheet ("color:#8C98A4");
}

MainWindow::~MainWindow() {
  delete user_menu;
  delete login_widget;
  delete userSetting;
  delete ui;
}

/**
   @brief side menu button click callback function
*/
void MainWindow::clickSideMenuButton()  {
  ShowChildWidget (user_menu);
}

/* 针对控件事件处理 */
bool MainWindow::eventFilter (QObject* object, QEvent* event)  {
  if (object == ui->SideMenuButton
      and event->type() == QEvent::MouseButtonRelease) {
    clickSideMenuButton();
  } else if (object == this ) {
    if (event->type() == QEvent::HideToParent) {
      AfterMath();
      emit Quit();
    }
  }
  return QMainWindow::eventFilter (object, event);
}

void MainWindow::resizeEvent (QResizeEvent* event)  {
  // 当窗口大小发生改变，通知正在显示的子窗口更新大小
  if (is_show_widget != nullptr)
    is_show_widget->update();
}

void MainWindow::mousePressEvent (QMouseEvent* event)  {
  QWidget* w;
  w = childAt (event->pos());
  /* is_show_widget 保存正在显示子控件,当w(鼠标按下的控件) 的父控件
     不是is_show_widget 则隐藏的当前子控件
  */
  if (is_show_widget and is_show_widget->isVisible()) {
    is_show_widget->hide();
    is_show_widget = nullptr;
  }
}

// 初始化网络通信
void MainWindow::InitNetWork()  {
  network = new NetWork ("127.0.0.1", 6667, this);
  login_widget->setNetwork (network);
  QObject::connect (network, &NetWork::is_connected, this,
                    &MainWindow::NetWorkConnection );
  QObject::connect (network, &NetWork::newMessage, this,
                    &MainWindow::NetWorkMessagePressed);
  QObject::connect (this, &MainWindow::LoginStatusChanged, this,
                    &MainWindow::TriggerLoginChanged);
  QObject::connect (login_widget, &LoginWidget::LoginSendMessage, this,
                    &MainWindow::Login);
}

void MainWindow::NetWorkConnection (bool status)  {
  QString message[2] = {"连接服务器失败,请检查当前网络状况", "连接服务器成功: 正在验证用户是否有效..."};
  ui->statusbar->showMessage (message[status]);
  if (not status) { return; }
  // 没有找到登陆账号，直接发起登陆失败，显示登陆界面
  if (new_user_account.isNull()) {
    emit LoginStatusChanged (false);
    return;
  }
  // 向服务器发起登陆
  QString account = new_user_account;
  QString password = user[new_user_account].user_password;
  Login (account, password);
}

// 有新的消息信号被触发,读取消息
void MainWindow::NetWorkMessagePressed ()  {
  QByteArray message;
  network->RecvMessage (message);
  Map ma = parseMessageRequestHeaders (message.toStdString());
  if (ma["state"] == "200" and ma["message_type"] == "login") {
    ui->statusbar->showMessage ("登陆成功");
    // 保存新的账号信息
    qDebug() << "successful: lgoin";
    use_user_account = new_user_account;
    user[new_user_account].Save (*this->userSetting);
    emit LoginStatusChanged (true);
  } else {
    ui->statusbar->showMessage ("登陆失败，准备重新登陆");
    qDebug() << "failure: login";
    emit LoginStatusChanged (false);
  }
}

// 账户登陆状态被修改
void MainWindow::TriggerLoginChanged (bool status)  {
  if (status) {
    // 登陆成功
    if (login_widget->GetIsShow()) {
      login_widget->hide();
    }
    if (login_widget->GetIsShow()) {
      HideChildWidget (login_widget);
    }
    LoadServerAccountInformation();
  } else {
    // 登陆失败
    if (not login_widget->GetIsShow()) {
      ShowChildWidget (login_widget);
    }
  }
}

// 显示子窗口
void MainWindow::ShowChildWidget (QWidget* widget)  {
  if (is_show_widget != nullptr)
    is_show_widget->hide();
  is_show_widget = widget;
  is_show_widget->show();
}

// 加载用户数据
void MainWindow::LoadServerAccountInformation()  {
  ui->statusbar->showMessage ("正在加载用户数据...");
}

// 向服务器发起登陆请求
void MainWindow::Login (QString user_account, QString user_password)  {
  network->SendMessage (QMap<QString, QString> {
    {"type", "POST"},
    {"user_account", user_account},
    {"user_password", user_password},
    {"message_type", "login"}
  });
  User u;
  u.user_account = user_account;
  u.user_password = user_password;
  user[user_account] = u;
  new_user_account =  user_account;
}

// 隐藏子窗口
void MainWindow::HideChildWidget (QWidget* widget)  {
  if (is_show_widget != nullptr) {
    is_show_widget = nullptr;
    widget->hide();
  }
}

// 在程序关闭前,做的善后工作
void MainWindow::AfterMath()  {
  if (not use_user_account.isEmpty()) {
    userSetting->setValue ("account", use_user_account);
  }
}

// 获取上次退出时登陆的账号
QString MainWindow::GetLastLoginAccount()  {
  QVariant account =  userSetting->value ("account");
  if (account.isNull()) {
    return QString::Null();
  }
  return account.toString();
}

// 切换聊天频道
void MainWindow::SwitchChatChannel (QString Channel_id)  {

}
// TODO: 消息ui Demo未初始化
