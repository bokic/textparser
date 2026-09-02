// 10c: Statements, control flow, loops, and automatic semicolon insertion.
function control(): number {
    if (ready) return 1;
    else if (pending) return 2;
    else return 3;

    while (condition) value++;
    do value--; while (condition);

    for (let i = 0; i < 10; i++) log(i);
    for (;;) break;
    for (const key in object) process(key);
    for (const item of list) consume(item);

    switch (mode) {
        case 1:
            break;
        case 2:
        case 3:
            return 0;
        default:
            throw new Error("bad mode");
    }

    label: for (;;) {
        continue label;
    }

    try {
        risky();
    } catch (error) {
        recover();
    } finally {
        cleanup();
    }

    with (object) {
        inner();
    }

    const variableDeclarationWithAutomaticSemicolon = 1
    return valueWithoutSemicolon
         + more
}

if (a)
    singleStatement();
else
    otherStatement();

const asiBeforeClosingBrace = 5
while (asiLoop) {
    break
}