# M5RadioControl
DIY Radio Control system by M5Stack

![IMG_RC_SYSTEM](https://user-images.githubusercontent.com/64751855/154823502-b6f40bb0-9fc7-4578-9e62-ac9f5db41d3a.jpg)

- M5RadioControl（M5RC）は、RCシステムの送受信機TX/RXをM5Stackマイコンで自作するためのオープンソースソフトウェアです。
- RCシステムの標準機能（ペアリング、トリム、リバース、D/R、EPA、EXP設定など）に加えて、ロマン溢れる独自機能を実現しています。
- 独自機能とは、"ダンシングライダー"感覚の操縦機能、"走る実験室"感覚のIMUテレメータ表示／ロギング機能等です。
- 送信機TXは"M5Stack Gray"、受信機RXは"M5Atom Matrix/M5StickC"で動作確認しています。
- TX/RX間の通信は、"ESP-NOW"を利用して下り（TXからRXへ）約100Hz、上り（RXからTXへ）約300Hz、到達距離20m程度です。


# DEMO

M5RCシステムをタミヤのRCカーとミニ四駆へ搭載した事例です。

![M5RC_GR02](https://user-images.githubusercontent.com/64751855/155876897-721a2c08-705e-47fc-a46e-67b262cabae8.jpg)

![M5RC_M4WD](https://user-images.githubusercontent.com/64751855/155876951-76d9f351-90a1-456f-93d7-3befb422dc33.jpg)


# Features
M5RCシステムは、標準機能に加えて次のロマン溢れる（笑）独自機能を実現しています。

## ダンシング操縦機能
送信機TXのグリップ自体を「操縦桿」の様に傾けることでRCカーを操縦できます。
例えばタミヤT3-01に適用すると、貴方もダンシングライダーの世界に浸れます。


## テレメータ／ロギング機能
RCカー搭載IMUのテレメータ／ロギングにより、貴方専用の「走る実験室」を実現できます。
走行データの定量分析により、ホンダのF-1エンジニア気分でRCカーをセッティングできます。

![M4WD-plot](https://user-images.githubusercontent.com/64751855/155877157-9e4e1bb6-cacd-4e34-a1aa-a5ffe0449518.png)

![M4WD-traj](https://user-images.githubusercontent.com/64751855/155877205-44e6fe6a-db0b-4bdb-b37e-8a9ab7ada5a0.png)


## ステアリング・ジャイロ機能
RCカーのステアリング操作をPIDコントローラが優しくアシストしてスピンを防ぎます。
例えばドリフトRCカーに適用すると、安全かつ地球に優しく頭文字Ｄの世界に浸れます。


## ドレミファ・インバータ機能
ブラシモーターのスイッチング周波数変化を自在にプログラムできます。
例えばVVVFインバータ調サウンドにすると、貴方も電車運転士の気分です。


# Requirement

- 送信機TXハードウェアは、"M5Stack Gray"とA/Dコンバータ(I2C接続)
- 受信機RXハードウェアは、"M5Atom Matrix"または"M5StickC"
- ホビー用RCのサーボとESC（電子式スピードコントローラ）など


# Usage



# Reference


