# Reentrant Challenges Write-up
0816146 Sean 韋詠祥

## Challenge #1 - Play with the Oracles

透過 `diff oracle{,p1}.c` 發現，兩個檔案除了版本，差異只有 `gen_secret` 後儲存時有沒有使用 `strdup()` 函式。

```diff
--- oracle.c
+++ oraclep1.c
@@ -71,4 +71,4 @@
        if ((secret = gen_secret(o->key, userkey)) != NULL) {
-               o->secret = secret;
+               o->secret = strdup(secret);
                o->stage = (o->stage + 1) % maxstage;
        }
@@ -158,1 +158,1 @@
-               "Welcome to the INP Oracle System v0.0\n\n"
+               "Welcome to the INP Oracle System v0.0p1\n\n"
```

因此猜測兩邊的 `secret` 共用，實際嘗試後成功利用 Oracle 1 揭曉的 secret 輸入到 Orcale 2 並成功拿到 flag。

## Challenge #2 - Play with the Oracles (patch1)

觀察程式碼發現，只有在 `oseed < 0` 時才會重新呼叫 `srand()`，而 `oseed` 只有在顯示 secret 時重置。

```c
char *gen_secret(int k1, int k2) {
    if (oseed < 0) {
        oseed = (magic ^ k1 ^ k2) & 0x7fffffff;
        srand(oseed);
    }
}
```

因此前後開啟 Oracle 1 及 Oracle 2 使用的是同一組狀態，只要拿到 Oracle 1 的 `oseed`，就能推算出 Oracle 2 的 `secret` 為何。

或是先顯示 Oracle 2 的 `secret` 及 `oseed`，對於推算 Oracle 1 `secret` 的做法會更為直觀。

## Challenge #3 - Web Crawler

由於分成 2 個 thread 執行，並且在 `do_job()` 時沒有 lock，會導致變數互相覆蓋。

利用位址不合法時不會清除的特性，搭配嘗試至多兩次的機制，就能在重新嘗試時對原先被禁止的地址連線。
