基于Android与STM32的宿舍自动开门神器
===
## 一、概述
一直想做一个，能够自动开宿舍门的东西，前几天看到网上的一个帖子，有人已经做过了，用的是电磁铁，我就开始动手了。目的在于，可以免除带钥匙，可以在床上开门。

## 二、材料
* STM32F103C8T6
* C-60-12直流电源（OUTPUT：+12v）
* LM2596降压模块（OUTPUT：+5v）
* BT-05蓝牙4.0模块
* 电磁铁推拉式长行程35mm微型直流12v24v220V牵引吸力**6kg**电磁门锁 http://b.mashort.cn/S.b587vF *（注意，要选用大吸力的电磁铁，否则无法拉开门闩）*
* 继电器模块
* 细线
## 三、思路
主要想法就是通过手机蓝牙串口，发送到BT-05蓝牙串口模块，然后单片机读取发送的数据，若校验成功，则给继电器高电平，电磁铁通电，拉开门闩。主要分为STM32部分和Android部分。

## 四、实现
### （一）、STM32
这里代码很简单，主要就是串口通讯。直接贴代码：

    while (1)
	{
		delay_ms(10);
		while (!(USART_RX_STA&0x8000))
		{
			continue;
		}
		USART_RX_STA = 0;
		if ((USART_RX_BUF[0] != '4')
			||(USART_RX_BUF[1] != '5')
			||(USART_RX_BUF[2] != '1'))
		{
			continue;
		}
		LED = !LED;
		DOOR = 1;
		USART_RX_STA = 0;
		delay_ms(1000);
		delay_ms(1000);
		DOOR = 0;
	}
没有什么好说的，很简单。
### （二）、Android
原本，这个也很简单，就是蓝牙串口，用之前的代码，很简单的。但是，我有一个更高的需求，那就是：越简单越好，越少点击次数越好，所以我想，如果能添加一个桌面小部件，点一下桌面小部件，就发送蓝牙串口消息，那岂不方便很多，都不需要进入activity，于是我就学习了半天关于桌面小部件（Widget）的相关知识。其实也不难，先新建一个widget，然后在继承自AppWidgetProvider的类，默认是NewAppWidget这个类里面的updateAppWidget函数里面，为button添加事件。

    static void updateAppWidget(Context context, AppWidgetManager appWidgetManager,
                                int appWidgetId) {

        CharSequence widgetText = context.getString(R.string.appwidget_text);
        RemoteViews views = new RemoteViews(context.getPackageName(), R.layout.new_app_widget);
        Intent intentservice = new Intent();
        intentservice.setClass(context, AppWidgetService.class);
        PendingIntent pendingIntent = PendingIntent.getService(context, 0, intentservice, 0);
        views.setOnClickPendingIntent(R.id.button, pendingIntent);
        appWidgetManager.updateAppWidget(appWidgetId, views);
    }
这样就为id为button的Button绑定了一个service，点击button，就会启动AppWidgetService这个Service。下面我们来看一下AppWidgetService，我们的这个程序的最主要的部分就在这个服务里面。
    package com.zhang.door;

    import android.app.Service;
    import android.bluetooth.BluetoothDevice;
    import android.content.BroadcastReceiver;
    import android.content.ComponentName;
    import android.content.Context;
    import android.content.Intent;
    import android.content.IntentFilter;
    import android.content.ServiceConnection;
    import android.os.IBinder;
    import android.util.Log;
    import android.widget.Toast;
    import static android.content.ContentValues.TAG;

    public class AppWidgetService extends Service {

        private final static String TAG = DeviceControlActivity.class.getSimpleName();

        public static final String EXTRAS_DEVICE_NAME = "DEVICE_NAME";
        public static final String EXTRAS_DEVICE_ADDRESS = "DEVICE_ADDRESS";

        private String mDeviceName = "451";
        private BluetoothLeService mBluetoothLeService;
        private boolean mConnected = false;
        private String mDeviceAddress = "00:15:83:00:77:AF";

        public AppWidgetService() {
        }
        private final ServiceConnection mServiceConnection = new ServiceConnection() {

            @Override
            public void onServiceConnected(ComponentName componentName, IBinder service) {
                mBluetoothLeService = ((BluetoothLeService.LocalBinder) service).getService();
                if (!mBluetoothLeService.initialize()) {
                    Log.e(TAG, "Unable to initialize Bluetooth");
                }

                Log.e(TAG, "mBluetoothLeService is okay");
                // Automatically connects to the device upon successful start-up initialization.
                //mBluetoothLeService.connect(mDeviceAddress);
                mBluetoothLeService.connect(mDeviceAddress);
            }

            @Override
            public void onServiceDisconnected(ComponentName componentName) {
                mBluetoothLeService = null;
            }
        };


        private final BroadcastReceiver mGattUpdateReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                final String action = intent.getAction();
                if (BluetoothLeService.ACTION_GATT_CONNECTED.equals(action)) {
                    Log.e(TAG, "Only gatt, just wait");
                } else if (BluetoothLeService.ACTION_GATT_DISCONNECTED.equals(action)) {
                    mConnected = false;

                }else if(BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED.equals(action))
                {
                    mConnected = true;
    //                ShowDialog();
                    Log.e(TAG, "In what we need");
                    mBluetoothLeService.WriteValue("451\r\n");
                    mBluetoothLeService.disconnect();
                    stopSelf();
                }
            }
        };

        @Override
        public IBinder onBind(Intent intent) {
            // TODO: Return the communication channel to the service.
            Log.w(TAG, "onBind");
            throw new UnsupportedOperationException("Not yet implemented");
        }

        @Override
        public int onStartCommand(Intent intent, int flags, int startId) {
            Log.w(TAG, "onStartCommand: hello world!");
            Intent gattServiceIntent = new Intent(this, BluetoothLeService.class);
            Log.d(TAG, "Try to bindService=" + bindService(gattServiceIntent, mServiceConnection, BIND_AUTO_CREATE));

            registerReceiver(mGattUpdateReceiver, makeGattUpdateIntentFilter());
            Toast.makeText(getApplicationContext(), "开门", Toast.LENGTH_LONG).show();

            return super.onStartCommand(intent, flags, startId);
        }

        @Override
        public void onDestroy() {
            super.onDestroy();
            unbindService(mServiceConnection);
            unregisterReceiver(mGattUpdateReceiver);
        }

        private void ShowDialog()
        {
            Toast.makeText(this, "连接上啦！", Toast.LENGTH_SHORT).show();
        }

        private static IntentFilter makeGattUpdateIntentFilter() {
            final IntentFilter intentFilter = new IntentFilter();
            intentFilter.addAction(BluetoothLeService.ACTION_GATT_CONNECTED);
            intentFilter.addAction(BluetoothLeService.ACTION_GATT_DISCONNECTED);
            intentFilter.addAction(BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED);
            intentFilter.addAction(BluetoothLeService.ACTION_DATA_AVAILABLE);
            intentFilter.addAction(BluetoothDevice.ACTION_UUID);
            return intentFilter;
        }
    }
在onStartCommand方法里面，绑定了BluetoothLeService，注册了mGattUpdateReceiver，在这里有一个回调方法onServiceConnected，当服务一旦绑定成功，在这个方法里面就会调用mBluetoothLeService的connect方法，尝试连接到蓝牙串口模块。如果连接成功，会进入BroadcastReceiver里面，如果连接上，就会嗲用mBluetoothLeService的WriteValue方法，发送”451\r\n”，然后蓝牙模块接收到，单片机就会处理，这时候电磁铁上电，门闩就被拉开了。
## 五、效果图
![演示]("https://github.com/rty813/451door/blob/master/demo.gif")
