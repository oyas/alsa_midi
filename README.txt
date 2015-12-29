ALSA_MIDI

キーボードの入力をMIDI信号に変えて送信するプログラム。
入力キーと出力MIDI番号の対応は、setting.txtに書きます。
Raspberry Pi で動かすことを想定していますが、普通のUbuntu等でも動きます。


コンパイル方法:

必要なライブラリのインストール
$ sudo apt-get install libasound2-dev

コンパイル
$ gcc alsa_midi.c -lasound -o alsa_midi




ファイルの説明：

alsa_midi.c		プログラムのソースコード
setting.txt		設定ファイル

install手順		Raspberry Pi へのインストール方法。自動起動の設定も含む。



