/*
 * UiRender.re
 *
 * Core render logic for a UI bound to a Window
 */

open Revery_Core;
open Revery_Draw;
open Revery_Math;

module Layout = Layout;
module LayoutTypes = Layout.LayoutTypes;

let _projection = Mat4.create();

open RenderContainer;

let render =
    (
      ~forceLayout=false,
      container: RenderContainer.t,
      component: React.syntheticElement,
    ) => {
  let renderContainer = container;
  let {rootNode, window, container, _} = container;

  AnimationTicker.tick();

  /* Perform reconciliation */
  Performance.bench("reconcile", () =>
    container := Container.update(container^, component)
  );

  /* Layout */
  let size = Window.getRawSize(window);
  let pixelRatio = Window.getDevicePixelRatio(window);
  let scaleFactor = Window.getScaleAndZoom(window);
  let adjustedHeight =
    float_of_int(size.height) /. scaleFactor |> int_of_float;
  let adjustedWidth = float_of_int(size.width) /. scaleFactor |> int_of_float;

  RenderContainer.updateCanvas(~width=size.width, ~height=size.height, renderContainer);

  rootNode#setStyle(
    Style.make(
      ~position=LayoutTypes.Relative,
      ~width=adjustedWidth,
      ~height=adjustedHeight,
      (),
    ),
  );
  Layout.layout(~force=forceLayout, rootNode);

  /* Recalculate cached parameters */
  Performance.bench("recalculate", () => rootNode#recalculate());

  /* Flush any node callbacks */
  Performance.bench("flush", () => rootNode#flushCallbacks());

  /* Render */
  Performance.bench("draw", () => {
    /* Do a first pass for all 'opaque' geometry */
    /* This helps reduce the overhead for the more expensive alpha pass, next */

    switch (renderContainer.canvas^) {
    | None => ();
    | Some(canvas) => 
      Revery_Draw.Canvas.test_draw(canvas);
    
     let drawContext = NodeDrawContext.create(~canvas, ~zIndex=0, ~opacity=1.0, ());

    /*Mat4.ortho(
      _projection,
      0.0,
      float_of_int(adjustedWidth),
      float_of_int(adjustedHeight),
      0.0,
      1000.0,
      -1000.0,
    );*/

    // let drawContext = NodeDrawContext.create(~zIndex=0, ~opacity=1.0, ());

    RenderPass.start(
      ~canvas,
      ~pixelRatio,
      ~scaleFactor,
      ~screenHeight=adjustedHeight,
      ~screenWidth=adjustedWidth,
      ~projection=_projection,
      (),
    );
    rootNode#draw(drawContext);
    //DebugDraw.draw();
    RenderPass.endAlphaPass();

    Revery_Draw.Canvas.flush(canvas);
    };
  });
};
