/**
 * The handler type used to type the optional accumulator function
 * returned by useSubscription. handler is a GADT used to support
 * proper type inference for useSubscription.
 */;
type handler('acc, 'response, 'ret) =
  | Handler((option('acc), 'response) => 'acc)
    : handler('acc, 'response, 'acc)
  | NoHandler: handler('response, 'response, 'response);

type useSubscriptionArgs = {
  query: string,
  variables: Js.Json.t,
  pause: option(bool),
  context: Types.partialOperationContext,
};

type executeSubscriptionJs = Types.partialOperationContext => unit;

[@bs.module "urql"]
external useSubscriptionJs:
  (useSubscriptionArgs, option((option('acc), Js.Json.t) => 'acc)) =>
  (Types.jsHookResponse('ret, 'extensions), executeSubscriptionJs) =
  "useSubscription";

type executeSubscription =
  (
    ~additionalTypenames: array(string)=?,
    ~fetchOptions: Fetch.requestInit=?,
    ~fetch: (string, Fetch.requestInit) => Js.Promise.t(Fetch.response)=?,
    ~requestPolicy: Types.requestPolicy=?,
    ~url: string=?,
    ~pollInterval: int=?,
    ~meta: Types.operationDebugMeta=?,
    ~suspense: bool=?,
    ~preferGetMethod: bool=?,
    unit
  ) =>
  unit;

type useSubscriptionResponse('response, 'extensions) = (
  Types.hookResponse('response, 'extensions),
  executeSubscription,
);

let subscriptionResponseToReason =
    (result: Types.jsHookResponse('ret, 'extensions)) => {
  let data = result.data;
  let error =
    result.error->Belt.Option.map(CombinedError.combinedErrorToRecord);
  let Types.{operation, fetching, extensions, stale} = result;

  let response =
    switch (fetching, data, error) {
    | (true, None, None) => Types.Fetching
    | (_, Some(d), None) => Data(d)
    | (_, Some(d), Some(e)) => PartialData(d, e.graphQLErrors)
    | (_, None, Some(e)) => Error(e)
    | (false, None, None) => Empty
    };

  Types.{operation, fetching, data, error, response, extensions, stale};
};

let useSubscription =
    (
      type acc,
      type response,
      type ret,
      ~request: Types.request(response),
      ~handler: handler(acc, response, ret),
      ~pause=?,
      ~additionalTypenames=?,
      ~fetchOptions=?,
      ~fetch=?,
      ~requestPolicy=?,
      ~url=?,
      ~pollInterval=?,
      ~meta=?,
      ~suspense=?,
      ~preferGetMethod=?,
      (),
    ) => {
  let query = request##query;
  let variables = request##variables;
  let parse = request##parse;
  let context = {
    Types.partialOperationContext(
      ~additionalTypenames?,
      ~fetchOptions?,
      ~fetch?,
      ~requestPolicy=?Belt.Option.map(requestPolicy, Types.requestPolicyToJs),
      ~url?,
      ~pollInterval?,
      ~meta?,
      ~suspense?,
      ~preferGetMethod?,
      (),
    );
  };

  let args = {query, variables, pause, context};

  let h = (acc, subscriptionResult) =>
    switch (handler) {
    | Handler(handlerFn) => handlerFn(acc, parse(subscriptionResult))
    | NoHandler => parse(subscriptionResult)
    };

  let (responseJs, executeSubscriptionJs) =
    useSubscriptionJs(args, Some(h));

  let response = subscriptionResponseToReason(responseJs);
  let executeSubscription =
    React.useMemo1(
      (
        (),
        ~additionalTypenames=?,
        ~fetchOptions=?,
        ~fetch=?,
        ~requestPolicy=?,
        ~url=?,
        ~pollInterval=?,
        ~meta=?,
        ~suspense=?,
        ~preferGetMethod=?,
        (),
      ) => {
        let ctx =
          Types.partialOperationContext(
            ~additionalTypenames?,
            ~fetchOptions?,
            ~fetch?,
            ~requestPolicy=?
              Belt.Option.map(requestPolicy, Types.requestPolicyToJs),
            ~url?,
            ~pollInterval?,
            ~meta?,
            ~suspense?,
            ~preferGetMethod?,
            (),
          );
        executeSubscriptionJs(ctx);
      },
      [|executeSubscriptionJs|],
    );

  (response, executeSubscription);
};
