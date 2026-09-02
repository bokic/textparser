// 10f: JSX/TSX elements.
const element1 = <div className="container" data-id={42} />;
const element2 = <Component x={value} y="text" />;
const element3 = <div><span>hello</span><span>world</span></div>;
const element4 = <ul>{items.map((item) => <li key={item}>{item}</li>)}</ul>;
const element5 = <App>
    <Header />
    <Body>
        <p>text content</p>
        {condition && <Footer />}
    </Body>
</App>;
const fragment = <><div>a</div><div>b</div></>;
const spread = <Card {...props} extra={1} />;
const dotted = <namespace.Component attr="value" />;
const namespacedAttr = <svg:path xlink:href="#icon" stroke="currentColor" />;
const namespacedValue = <tag ns:attr={value} other="fixed" />;
const selfClosing = <input type="text" value={text} />;
const dynamicAttr = <div {...rest} a={1} b="2" />;
const textWithExpr = <div>before {interpolation} after</div>;
const nestedFragments = <><><span>deep</span></></>;
const mixedChildren = <p>text <strong>bold</strong> tail</p>;

type Props = { text: string; count: number };
function render(props: Props) {
    return <div class="result">{props.text}: {props.count}</div>;
}

const comments = <div>{/* block comment */}</div>;
const multiline = <div
    className="wide"
    style={styles}
>
    content
</div>;

export const jsxValue = <Button disabled={!enabled} onClick={handle} />;