// 参考にしたサイト↓
// http://www.nurs.or.jp/~sug/soft/binbo/binbo5.htm

// 基本的には、このソースをいじる必要はなくて、setting.txtで設定をいじれるようにしています。

// 編集した場合は、raspberry pi を起動すれば自動でコンパイル・実行してくれるので、保存するだけで大丈夫です。
// (その設定は、.bashrcに書いてある)

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <alsa/asoundlib.h>


// 終了コマンド(この文字列が入力されるとRaspberry piの電源を落とす処理をします)
#define SHUTDOWN_COMMAND "shutdown0"

/* 接続先（実際のハードウェアデバイスの番号）*/
// 初期値は20:1。setting.txtで設定可能
int out_client = 20;
int out_port = 1;

// 設定ファイル名
#define SETTING_FILENAME "setting.txt"

// keymap配列のサイズ(固定)
#define KEYMAPSIZE 256

/* 自分自身のポート番号 */
int my_client;
int my_port;

/* アクセス構造体 */
snd_seq_t *handle = NULL;
snd_seq_event_t ev;

int keymap[KEYMAPSIZE];	// キーとMIDI番号の対応表

#define DEFAULT_NAME "alsa_midi"

/* シーケンサの初期設定 */
static int seq_open( void ) 
{
	unsigned int caps;
	int ret;

	/* 出力専用で開く */
	if( snd_seq_open( &handle, "hw", SND_SEQ_OPEN_OUTPUT, 0 ) < 0) {
		fprintf( stderr, "can't open sequencer device");
		return -1;
	}

	/* 自分自身のクライアント番号を取得し */
	my_client = snd_seq_client_id( handle );
	/* aconnect などのためにクライアント名をセットする */
	snd_seq_set_client_name( handle, DEFAULT_NAME );

	/* 自分自身のポートを作成する */
	my_port = snd_seq_create_simple_port( handle, DEFAULT_NAME, 
			SND_SEQ_PORT_CAP_READ,
			SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION );
	if( my_port < 0 ){
		fprintf( stderr, "can't create port\n" );
		snd_seq_close( handle );
		return -1;
	}

	/* 出力先に接続する */
	ret = snd_seq_connect_to( handle, my_port, out_client, out_port );
	if( ret < 0 ) {
		fprintf( stderr, "can't subscribe to MIDI port (%d:%d)\n", 
				out_client, out_port);
		snd_seq_close( handle );
		return -1;
	}
	return 0;
}

/* クローズ処理 */
static void seq_close( )
{
	snd_seq_close( handle );
}

/* 実際にイベントを送る下請け関数 */
static void send_event(int do_flush)
{
	snd_seq_ev_set_direct( &ev );
	snd_seq_ev_set_source( &ev, my_port );
	snd_seq_ev_set_dest( &ev, out_client, out_port );

	snd_seq_event_output( handle, &ev );
	if (do_flush) {
		snd_seq_drain_output( handle ); /* フラッシュする */
	}
}

/* NOTE ON イベントを送る */
static void note_on( int ch, int note, int vel)
{
	snd_seq_ev_set_noteon( &ev, ch, note, vel );
	send_event(1);
}

/* NOTE OFF イベントを送る */
static void note_off( int ch, int note, int vel)
{
	snd_seq_ev_set_noteoff( &ev, ch, note, vel );
	send_event(1);
}


// 設定ファイルを読み込んでkeymapを設定する
int setFromFile(int keymap[KEYMAPSIZE]){
	FILE *fp;
	char c, str[1024];
	int key, note;

	fp = fopen( SETTING_FILENAME, "r" );
	if ( fp == NULL ){
		return 1;
	}
	printf("keymap setting...\n");
	// 読み取り作業
	while ( fgets(str, 1024, fp) ){
		key = -1;
		note = -1;
		sscanf(str, "%c", &c);
		if( c == '#' || c == '/'){
			// コメント行なので無視
			continue;
		}else if( c == '@' ){
			// midiクライアント・ポートの設定の読み込み
			// @c... : [出力midiクライアントの番号]
			// @p... : [出力midiクライアントのport番号]
			char st[256];
			int num;
			sscanf(str, "%s : %d", st, &num);
			if( st[1] == 'c' ){
				out_client = num;
			}else if( st[1] == 'p' ){
				out_port = num;
			}
		}else if( c == '\'' ){
			// 入力キー : 出力キー番号
			// ただし、入力キーのasciiコード番号と入力キー番号が一致する場合のみこの設定の仕方ができる
			sscanf(str, "'%c' : %d", &c, &note);
			key = (int)c;
		}else{
			// 入力キー番号 : 出力キー番号
			sscanf(str, "%d : %d", &key, &note);
		}
		if( 0 < key && key < KEYMAPSIZE ){
			keymap[key] = note;
		}
	}
	fclose( fp );

	return 0;
}

int main( ) {
	int i, input, cnt=0, num;
	char shutdown[] = SHUTDOWN_COMMAND;
	
	// キーマップの初期化
	for(i=0; i<KEYMAPSIZE; i++)
		keymap[i] = -1;
	setFromFile(keymap);	//設定ファイルを読み取って設定
	
	if( seq_open() != 0 ) {
		return 1;
	}
	
	system("stty raw");	// リアルタイム入力に切換
	printf("input: ");
	while( (input = getchar()) != EOF ) {	// EOFの入力があるまでループ。永遠にループ抜けなくていいやって
		if( keymap[input] >= 0 ){
			note_on( 0, keymap[input], 64 );  /* midi信号を送信 */
			usleep( 1000 );            /* 1ミリ秒待つ */
			note_off( 0, keymap[input], 64 ); /* 消音(いらないかもね) */
		}
		// シャットダウン処理
		if( input == shutdown[cnt] ){
			cnt++;
			if( shutdown[cnt] == '\0' ){
				system("stty -raw");
				printf("\nshutdown now...\n");
				// シャットダウンしていることを表すmidi信号
				for(num=24; num>=20; num--){
					note_on( 0, num, 64 );
					usleep( 400000 );            /* 400ミリ秒待つ */
					note_off( 0, num, 64 );
				}
				system("sudo shutdown now");
				break;
			}
		}else cnt=0;
		// ログ出力処理
		system("stty -raw");
		printf(" %d\t\toutput: %d\n", input, keymap[input]);
		system("stty raw");
		printf("input: ");
	}
	system("stty -raw");	// リアルタイム入力をやめる
	seq_close();
	return 0;
}
