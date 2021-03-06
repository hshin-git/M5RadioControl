# M5RadioControl
DIY Radio Control system by M5Stack

![IMG_RC_SYSTEM](https://user-images.githubusercontent.com/64751855/154823502-b6f40bb0-9fc7-4578-9e62-ac9f5db41d3a.jpg)

- [M5RadioControl（M5RC）](https://github.com/hshin-git/M5RadioControl)は、RCシステムの送信機TX/受信機RXをM5Stackで自作するためのオープンソースソフト（OSS）です。
- RCシステムの標準機能（ペアリング、モデルメモリ、トリム、リバース、D/R、EPA、EXP等）に加えて独自機能を実現します。
- 独自機能は、「ダンシングライダー」気分の操縦機能、「走る実験室」気分のテレメータ／ロギング機能等です。
- 送信機TXは「[M5Stack Gray](https://docs.m5stack.com/en/core/gray)」、受信機RXは「[M5Atom Matrix](https://docs.m5stack.com/en/core/atom_matrix)」と「[M5StickC](https://docs.m5stack.com/en/core/m5stickc)」で動作確認済みです。
- TX/RX間の通信は、「[ESP-NOW](https://github.com/espressif/esp-now)」を利用して、下り（TXからRXへ）約100Hz、上り（RXからTXへ）約300Hz、到達距離20m程度です。


# DEMO

M5RCシステムをタミヤ製のRCカー（グラスホッパー２）とミニ四駆（FM-Aシャーシ）へ搭載した事例です。

![M5RC_GR02](https://user-images.githubusercontent.com/64751855/155876897-721a2c08-705e-47fc-a46e-67b262cabae8.jpg)
![M5RC_M4WD](https://user-images.githubusercontent.com/64751855/155876951-76d9f351-90a1-456f-93d7-3befb422dc33.jpg)


# Features
~~退屈な~~標準機能の解説をすっ飛ばして、独自機能の「ロマン溢れる使い方」を解説します。

## ダンシング・コントロール機能
送信機TXのグリップ自体を「操縦桿」の様に傾けることでRCカーを操縦できます。
例えば[タミヤT3-01](https://www.tamiya.com/japan/products/57405/index.html)を操縦すると、貴方は「ダンシングライダー（オジサン）」の世界に没入できます。

![T3-01](https://d7z22c0gz59ng.cloudfront.net/japan_contents/img/usr/item/5/57405/57405_1.jpg)


## テレメータ／ロギング機能
RCカー搭載IMU（慣性計測ユニット）のテレメータ／ロギングにより、貴方専用の「走る実験室」をお手軽に実現できます。
走行データの定量分析により「ホンダF1のエンジニア」気分でデータドリブンにマシンをセッティングできます。

![M4WD-plot](https://user-images.githubusercontent.com/64751855/155877157-9e4e1bb6-cacd-4e34-a1aa-a5ffe0449518.png)
![M4WD-traj](https://user-images.githubusercontent.com/64751855/156074555-eef4edac-f4a1-41a3-a283-c758bf34b154.png)

こちらは[ミニ四駆コース](https://genkikkosan.com/)の走行データ例（三週目の立体交差でコースアウト）です。
IMUデータは、シャーシ固定座標系に対する成分表示で、右がx成分、前がy成分、上がz成分です。
3D軌跡は、車速vyがスロットル（ch2）に比例すると仮定してAHRS計算値（pitch、yaw）を積算した結果です。
AHRS計算値に少しバイアスが乗っていますが、走行データから三次元的なコースレイアウトが分かります。


## ステアリング・アシスト機能
RCカーのステアリング操作をPIDコントローラが素早くアシストしてスピンを防ぎます。
例えば[ヨコモYD-2](https://teamyokomo.com/product/dp-yd2/)を操縦すると、貴方は安全かつ地球に優しく「頭文字Ｄ」の世界に浸れます。

![M5RC_DRIFT](https://user-images.githubusercontent.com/64751855/156068585-76c348eb-bc47-495f-889b-ec987f2f0023.jpg)

ソースコードは、こちらの「[ラジドリ用ジャイロGyroM5](https://protopedia.net/prototype/2351)」を流用しています。
PIDコントローラのパラメータ調整は、プロポ（送信機）側メニューから設定すると即反映のお手軽さです。


## ドレミファ・インバータ機能
ブラシモーターのスイッチング周波数をタイマとスロットルに応じてプログラムできます。
例えばVVVFインバータ調サウンドにプログラムすると、貴方は電車運転士の気分です。

![RAILWAY_HDR](https://user-images.githubusercontent.com/64751855/156074703-8c3c4c0f-50f9-492a-83f9-2223110b4df2.jpg)



# Requirement

## Software
- M5RCのFW開発環境: 「[ArduinoIDE](https://www.arduino.cc/en/software)」（必須、[M5TX](/M5TX)/[M5RX](/M5RX)コンパイル&マイコン転送用）
- IMUデータ分析環境: 「[Python/Anaconda](https://www.anaconda.com/products/individual)」（テレメータ／ロギング機能利用時のみ、[tool](/tool)実行用）


## Hardware
- M5RC送信機TX: 「[M5Stack Gray](https://docs.m5stack.com/en/core/gray)」「A/Dコンバータ(I2C接続)」と「RC送信機の筐体（コントローラ）」
- M5RC受信機RX: 「[M5Atom Matrix](https://docs.m5stack.com/en/core/atom_matrix)」または「[M5StickC](https://docs.m5stack.com/en/core/m5stickc)」と「RC受信機の配線（ワイヤハーネス）」
- ホビー用RCカー: RCカーのシャーシ、ステアリング用サーボとスロットル用ESC（電子式スピードコントローラ）

![M5RC-System](https://user-images.githubusercontent.com/64751855/156860549-5ae9e112-885b-40f1-bd0b-b161a9fb3487.png)



# Installation

## Sotware
- M5RC送信機TX: ファームウェア[M5TX](/M5TX)を[ArduinoIDE](https://www.arduino.cc/en/software)でコンパイルして「[M5Stack Gray](https://docs.m5stack.com/en/core/gray)」に転送します。
- M5RC受信機RX: ファームウェア[M5RX](/M5RX)を[ArduinoIDE](https://www.arduino.cc/en/software)でコンパイルして「[M5Atom Matrix](https://docs.m5stack.com/en/core/atom_matrix)」に転送します。


## Hadware

### M5TXの配線
M5Stackは、GROVEポート経由でADコンバータと接続します。
M5Stackの筐体は、適当な方法でRC送信機の筐体に固定してください。
例えばスマホ用メタルプレートをRC送信機に取り付けた上、M5Stackをマグネット留めすると着脱が容易です。

|M5Stack Gray |in/out |ADC |
|---- |---- |---- |
|GND  |out |GND |
|5V   |out |VDD |
|G21(SDA) |in/out |SDA |
|G22(SCL) |out    |SCL |

ADコンバータは、RC送信機TXのステアリング／スロットル位置検出用の可変抵抗（VRのA/B/C端子）と接続します。
ADコンバータの入力チャンネルA3は、電圧VDDの測定用に使うので正味のチャンネル数は3です。

|ADC |in/out |TX (VR) |
|---- |---- |---- |
|GND  |out |VR (A) |
|VDD  |out |VR (C) |
|A0   |in  |VR (B1) |
|A1   |in  |VR (B2) |
|A3   |in  |VR (C) |

なお標準的なADコンバータTI製ADS1115の場合、サンプリングレートが最大860回/秒なので、3ch計測のとき最大860/3=286回/秒です。
これにI2C通信やパケット送信の時間を加えた結果、最終的なM5TXの送信周期は約100回/秒でした。
より高速なADコンバータを利用すれば、M5TXの送信周期は更に高められそうです。


### M5RXの配線
M5Atomは、ホビー用RCユニット（サーボ、ESC）の電源及び信号線（-/+/S）と接続します。
GROVE端子は、自作モータードライバ（H-bridge）のゲート信号出力用に使います。

|M5Atom |in/out |RC Units |
|---- |---- |---- |
|GND  |in |RC (-) |
|5V   |in |RC (+) |
|G22  |out |RC (S1) |
|G19  |out |RC (S2) |
|G26  |out |DIY-ESC |
|G32  |out |DIY-ESC |


M5StickCの場合、以下のように接続します。

|M5StickC |in/out |RC Units |
|---- |---- |---- |
|GND  |in |RC(-) |
|5Vin |in |RC(+) |
|G26  |out |RC(S1) |
|G0   |out |RC(S2) |
|G32  |out |DIY-ESC |
|G33  |out |DIY-ESC |



# Usage
多機能なソフトウェアなので、ぼちぼち追記します。

- 送信機TX画面: 左エリアがバラメータ表示（縦スクロール）、右エリアがコマンド／IMUデータ表示（2Dカーソル）です。
- 送信機TX操作: パラメータ選択時はボタン「[A]上へ、[C]下へ、[B]決定」、パラメータ調整時はボタン「[A]減らす、ボタン[C]増やす、[B]決定」の操作です。
- ペアリング: 受信機RX側はボタン[A]を押して起動、送信機TX側はメニュー「pairing」選択で、ペアリングモードに入ります。
- 注意点など: 送信機TX側はA/Dコンバータを外して電源オンすると起動中にブロックします（I2C通信にタイムアウトが無いので）。


# Reference

## ラジコン関係
- [ラジドリ用ジャイロGyroM5＠github](https://github.com/hshin-git/GyroM5)
- [ラジドリ用ジャイロGyroM5＠protopedia](https://protopedia.net/prototype/2351)
- [3000円で至高の二駆ドリジャイロをゲットできたけど少し反省した話＠note](https://note.com/nanami00/n/n3a1958d79433)

## 電子部品関係
- [M5Stack Gray@スイッチサイエンス](https://www.switch-science.com/catalog/3648/)
- [M5Atom Matix@スイッチサイエンス](https://www.switch-science.com/catalog/6260/)
- [4ch/16bit/I2CのADコンバータ@amazon](https://www.amazon.co.jp/dp/B01D0WSCNG)
- [大電流ブラシモータ用ドライバ@amazon](https://www.amazon.co.jp/dp/B00WSN98DC/)
- [小電流ブラシモータ用ドライバ@amazon](https://www.amazon.co.jp/dp/B071SJ4T9M/)

