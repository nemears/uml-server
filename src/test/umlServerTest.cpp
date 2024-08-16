#include "gtest/gtest.h"
#include "uml-server/umlServer.h"
#include "uml-server/umlClient.h"
#include "uml/uml-stable.h"
#include "test/umlSererTest.h"
#include <stdlib.h>
#include <thread>

using namespace UML;

class UmlServerTests : public ::testing::Test {};

TEST_F(UmlServerTests, clientsConnectToServerTest) {
    UmlClient client1;
    UmlClient client2;
}

TEST_F(UmlServerTests, postAndGetTest) {
    UmlClient client;
    Class& clazz = *client.create<Class>();
    ID clazzID = clazz.getID();
    clazz.setName("clazz");
    ASSERT_TRUE(client.loaded(clazz.getID()));
    client.release(clazz);
    Class& clazz2 = client.get(clazzID)->as<Class>();
    ASSERT_EQ(clazz2.getName(), "clazz");
}

TEST_F(UmlServerTests, basicEraseTest) {
    UmlClient client;
    Class& clazz = *client.create<Class>();
    ID clazzID = clazz.getID();
    client.erase(clazz);
    ASSERT_FALSE(client.loaded(clazzID));
}

TEST_F(UmlServerTests, bigMessageTest) {
    int numChildren = 125;
    UmlClient client;
    PackagePtr root = client.create<Package>();
    for (int i = 0; i < numChildren; i++) {
        root->getPackagedElements().add(*client.create<Package>());
    }
    ID id = root.id();
    root->setName("foo");
    client.release(*root);
    ASSERT_EQ(client.get(id)->as<Package>().getPackagedElements().size(), numChildren);
    client.release(*root);
    ASSERT_EQ(client.get(id)->as<Package>().getPackagedElements().size(), numChildren);
}

TEST_F(UmlServerTests, headTest) {
    UmlClient client;
    PackagePtr root = client.create<Package>();
    PackagePtr child = client.create<Package>();
    root->setName("root");
    child->setName("test");
    root->getPackagedElements().add(*child);
    client.release(*child);
    client.setRoot(root.ptr());
    // ASSERT_EQ(*root, client.get(""));
    ASSERT_EQ(client.get(""), root);
}

TEST_F(UmlServerTests, saveTest) {
    UmlClient client;
    PackagePtr newRoot = client.create<Package>();
    ClassPtr clazz = client.create<Class>();
    newRoot->getPackagedElements().add(*clazz);
    client.setRoot(newRoot.ptr());
    client.release(*clazz);
    clazz->setName("clzz");
    client.release(*clazz);
    client.save();
    // TODO can misbehave and still pass
    // TODO shutdown server and startup server somehow and check
}

void raceConditionThread(ID id) {
    UmlClient client;
    PackagePtr pckg = &client.get(id)->as<Package>();
    PackagePtr child = client.create<Package>();
    pckg->getPackagedElements().add(*child);
    client.release(*pckg);
    client.release(*child);
    // client.putAll();
}

TEST_F(UmlServerTests, raceConditionTest) {
    UmlClient client;
    PackagePtr pckg = client.create<Package>();
    pckg.release();
    pckg.aquire();
    std::thread rcThread(&raceConditionThread, pckg.id());
    PackagePtr child = client.create<Package>();
    pckg->getPackagedElements().add(*child);
    // client.putAll();
    client.release(*pckg);
    client.release(*child);
    rcThread.join();
    // TODO 
    pckg.release();
    pckg.aquire();
    ASSERT_EQ(pckg->getPackagedElements().size(), 1);
    // ASSERT_EQ(pckg->getPackagedElements().front(), *child);
}

TEST_F(UmlServerTests, badReferenceTest) {
    UmlClient client;
    ID clazzID;
    ID instID;

    // dereference clazz from inst by releasing both, changing clazz's id, and releasing, then aquiring inst
    {
        ClassPtr clazz = client.create<Class>();
        InstanceSpecificationPtr inst = client.create<InstanceSpecification>();
        inst->getClassifiers().add(*clazz);
        clazzID = clazz.id();
        instID = inst.id();
        clazz.release();
        inst.release();
    }
    {
        ClassPtr clazz = &client.get(clazzID)->as<Class>();
        clazz->setID(ID::randomID());
        clazz.release();
    }
    InstanceSpecificationPtr inst;
    ClassPtr clazz;
    ASSERT_NO_THROW(inst = &client.get(instID)->as<InstanceSpecification>());
    ASSERT_NO_THROW(clazz = &client.get(clazzID)->as<Class>());
    ClassifierPtr instClassifier;
    ASSERT_NO_THROW(instClassifier = inst->getClassifiers().front());
    ASSERT_EQ(clazz.id(), instClassifier.id());
    ASSERT_EQ(clazz.ptr(), instClassifier.ptr());
}

// activity edge integration tests
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ActivityEdgeTarget, OpaqueAction, ControlFlow, &ActivityEdge::getTarget, &ActivityEdge::setTarget)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ActivityEdgeSource, OpaqueAction, ControlFlow, &ActivityEdge::getSource, &ActivityEdge::setSource)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ActivityEdgeGuard, LiteralBool, ControlFlow, &ActivityEdge::getGuard, &ActivityEdge::setGuard)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ActivityEdgeWeight, LiteralBool, ControlFlow, &ActivityEdge::getWeight, &ActivityEdge::setWeight)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ObjectFlowTransformation, OpaqueBehavior, ObjectFlow, &ObjectFlow::getTransformation, &ObjectFlow::setTransformation)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ObjectFlowSelection, Activity, ObjectFlow, &ObjectFlow::getSelection, &ObjectFlow::setSelection)

// activity node integration tests
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ActivityNodeActivity, Activity, OpaqueAction, &ActivityNode::getActivity, &ActivityNode::setActivity)
// UML_SERVER_SET_INTEGRATION_TEST(ActivityNodeIncoming, ControlFlow, OpaqueAction, &ActivityNode::getIncoming)
// UML_SERVER_SET_INTEGRATION_TEST(ActivityNodeOutgoing, ControlFlow, OpaqueAction, &ActivityNode::getOutgoing)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(DecisionNodeDescisionInput, Activity, DecisionNode, &DecisionNode::getDecisionInput, &DecisionNode::setDecisionInput)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(DecisionNodeDecisionInputFlow, ObjectFlow, DecisionNode, &DecisionNode::getDecisionInputFlow, &DecisionNode::setDecisionInputFlow)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(JoinNodeJoinSpec, LiteralInt, JoinNode, &JoinNode::getJoinSpec, &JoinNode::setJoinSpec)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ObjectNodeSelection, Activity, CentralBufferNode, &ObjectNode::getSelection, &ObjectNode::setSelection)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ObjectNodeUpperBound, LiteralInt, DataStoreNode, &ObjectNode::getUpperBound, &ObjectNode::setUpperBound)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ActivityParameterNodeParameter, Parameter, ActivityParameterNode, &ActivityParameterNode::getParameter, &ActivityParameterNode::setParameter)
// UML_SERVER_SET_INTEGRATION_TEST(ExecutableNodeHandlers, ExceptionHandler, OpaqueAction, &ExecutableNode::getHandlers)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ExceptionHandlerProtectedNode, OpaqueAction, ExceptionHandler, &ExceptionHandler::getProtectedNode, &ExceptionHandler::setProtectedNode)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ExceptionHandlerHandlerBody, OpaqueAction, ExceptionHandler, &ExceptionHandler::getHandlerBody, &ExceptionHandler::setHandlerBody)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ExceptionHandlerExceptionInput, DataStoreNode, ExceptionHandler, &ExceptionHandler::getExceptionInput, &ExceptionHandler::setExceptionInput)
// UML_SERVER_SET_INTEGRATION_TEST(ExceptionHandlerExceptionTypes, Activity, ExceptionHandler, &ExceptionHandler::getExceptionTypes)
// UML_SERVER_SET_INTEGRATION_TEST(ActionLocalPreConditions, Constraint, OpaqueAction, &Action::getLocalPreconditions)
// UML_SERVER_SET_INTEGRATION_TEST(ActionLocalPostConditions, Constraint, OpaqueAction, &Action::getLocalPostconditions)
// UML_SERVER_SET_INTEGRATION_TEST(OpaqueActionInputValues, InputPin, OpaqueAction, &OpaqueAction::getInputValues)
// UML_SERVER_SET_INTEGRATION_TEST(OpaqueActionOutputValues, OutputPin, OpaqueAction, &OpaqueAction::getOutputValues)
// UML_SERVER_SET_INTEGRATION_TEST(OpaqueActionBodies, LiteralString, OpaqueAction, &OpaqueAction::getBodies)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ActionInputPinFromAction, OpaqueAction, ActionInputPin, &ActionInputPin::getFromAction, &ActionInputPin::setFromAction)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ValuePinValue, LiteralBool, ValuePin, &ValuePin::getValue, &ValuePin::setValue)
// UML_SERVER_SET_INTEGRATION_TEST(InvocationActionArguments, InputPin, CallBehaviorAction, &InvocationAction::getArguments)
// UML_SERVER_SET_INTEGRATION_TEST(CallActionResults, OutputPin, CallBehaviorAction, &CallAction::getResults)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(CallBehaviorActionBehavior, Activity, CallBehaviorAction, &CallBehaviorAction::getBehavior, &CallBehaviorAction::setBehavior)

// activity integration tests
// UML_SERVER_SET_INTEGRATION_TEST(ActivityNodes, OpaqueAction, Activity, &Activity::getNodes)
// UML_SERVER_SET_INTEGRATION_TEST(ActivityEdges, ControlFlow, Activity, &Activity::getEdges)
// UML_SERVER_SET_INTEGRATION_TEST(ActivityPartitions, ActivityPartition, Activity, &Activity::getPartitions)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ActivityGroupInActivity, Activity, ActivityPartition, &ActivityGroup::getInActivity, &ActivityGroup::setInActivity)
// UML_SERVER_SET_INTEGRATION_TEST(ActivityPartitionSubPartitions, ActivityPartition, ActivityPartition, &ActivityPartition::getSubPartitions)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ActivityPartitionSuperPartition, ActivityPartition, ActivityPartition, &ActivityPartition::getSuperPartition, &ActivityPartition::setSuperPartition)
// UML_SERVER_SET_INTEGRATION_TEST(ActivityPartitionNodes, OpaqueAction, ActivityPartition, &ActivityPartition::getNodes)
// UML_SERVER_SET_INTEGRATION_TEST(ActivityNodeInPartition, ActivityPartition, OpaqueAction, &ActivityNode::getInPartitions)
// UML_SERVER_SET_INTEGRATION_TEST(ActivityPartitionEdges, ControlFlow, ActivityPartition, &ActivityPartition::getEdges)
// UML_SERVER_SET_INTEGRATION_TEST(ActivityEdgeInPartion, ActivityPartition, ControlFlow, &ActivityEdge::getInPartitions)
// UML_SERVER_SINGLETON_INTEGRATION_TEST(ActivityPartitionRepresents, Property, ActivityPartition, &ActivityPartition::getRepresents, &ActivityPartition::setRepresents)

// association integration tests
UML_SERVER_SET_INTEGRATION_TEST(AssociationOwnedEnd, Property, Association, &Association::getOwnedEnds)
UML_SERVER_SINGLETON_INTEGRATION_TEST(PropertyOwningAssociation, Association, Property, &Property::getOwningAssociation, &Property::setOwningAssociation)
UML_SERVER_SET_INTEGRATION_TEST(AssociationNavigableOwnedEnd, Property, Association, &Association::getNavigableOwnedEnds)
UML_SERVER_SET_INTEGRATION_TEST(AssociationMemberEnd, Property, Association, &Association::getMemberEnds)
UML_SERVER_SINGLETON_INTEGRATION_TEST(PropertyAssociation, Association, Property, &Property::getAssociation, &Property::setAssociation)

// behavioredClassifier integration tests
UML_SERVER_SET_INTEGRATION_TEST(BehavioredClassifierOwnedBehavior, OpaqueBehavior, Class, &BehavioredClassifier::getOwnedBehaviors)
UML_SERVER_SINGLETON_INTEGRATION_TEST(BehavioredClassifierClassifierBehavior, OpaqueBehavior, Class, &BehavioredClassifier::getClassifierBehavior, &BehavioredClassifier::setClassifierBehavior)

// behavior integration tests
UML_SERVER_SINGLETON_INTEGRATION_TEST(BehaviorSpecification, Operation, OpaqueBehavior, &Behavior::getSpecification, &Behavior::setSpecification)
UML_SERVER_SET_INTEGRATION_TEST(BehavioralFeatureOwnedParameters, Parameter, Operation, &BehavioralFeature::getOwnedParameters)
UML_SERVER_SET_INTEGRATION_TEST(BehavioralFeatureMethods, OpaqueBehavior, Operation, &BehavioralFeature::getMethods)
UML_SERVER_SET_INTEGRATION_TEST(BehavioralFeatureRaisedExceptions, OpaqueBehavior, Operation, &BehavioralFeature::getRaisedExceptions)
UML_SERVER_SET_INTEGRATION_TEST(BehaviorParameters, Parameter, OpaqueBehavior, &Behavior::getOwnedParameters)

// classifier integration tests
UML_SERVER_SET_INTEGRATION_TEST(ClassifierGeneralization, Generalization, Class, &Classifier::getGeneralizations)
UML_SERVER_SET_INTEGRATION_TEST(ClassifierPowerTypeExtent, GeneralizationSet, Class, &Classifier::getPowerTypeExtent)
UML_SERVER_SINGLETON_INTEGRATION_TEST(ClassifierOwnedTemplateSignature, RedefinableTemplateSignature, Class, &Classifier::getOwnedTemplateSignature, &Classifier::setOwnedTemplateSignature)
UML_SERVER_SINGLETON_INTEGRATION_TEST(RedefinableTemplateSignatureClassifier, PrimitiveType, RedefinableTemplateSignature, &RedefinableTemplateSignature::getClassifier, &RedefinableTemplateSignature::setClassifier)
UML_SERVER_SINGLETON_INTEGRATION_TEST(ClassifierTemplateParameter_, ClassifierTemplateParameter, Class, &Classifier::getTemplateParameter, &Classifier::setTemplateParameter)
UML_SERVER_SINGLETON_INTEGRATION_TEST(ClassifierTemplateParameterParameteredElement, OpaqueBehavior, ClassifierTemplateParameter, &ClassifierTemplateParameter::getParameteredElement, &ClassifierTemplateParameter::setParameteredElement)
UML_SERVER_SET_INTEGRATION_TEST(ClassifierTemplateParameterConstrainingClassifiers, Enumeration, ClassifierTemplateParameter, &ClassifierTemplateParameter::getConstrainingClassifiers)
UML_SERVER_SET_INTEGRATION_TEST(RedefinableTemplateSignatureExtendedSignature, RedefinableTemplateSignature, RedefinableTemplateSignature, &RedefinableTemplateSignature::getExtendedSignatures)

// class integration tests
UML_SERVER_SET_INTEGRATION_TEST(StructuredClassifierOwnedAttributes, Property, Class, &StructuredClassifier::getOwnedAttributes)
UML_SERVER_SET_INTEGRATION_TEST(ClassOwnedAttributes, Property, Class, &Class::getOwnedAttributes)
UML_SERVER_SET_INTEGRATION_TEST(StructuredClassifierOwnedConnectors, Connector, Class, &StructuredClassifier::getOwnedConnectors)
UML_SERVER_SET_INTEGRATION_TEST(ClassNestingClassifiers, DataType, Class, &Class::getNestedClassifiers)
UML_SERVER_SET_INTEGRATION_TEST(ClassOwnedOperations, Operation, Class, &Class::getOwnedOperations)
UML_SERVER_SET_INTEGRATION_TEST(ClassOwnedReceptions, Reception, Class, &Class::getOwnedReceptions)
UML_SERVER_SINGLETON_INTEGRATION_TEST(PropertyClass, Class, Property, &Property::getClass, &Property::setClass)
UML_SERVER_SINGLETON_INTEGRATION_TEST(OperationClass, Class, Operation, &Operation::getClass, &Operation::setClass)

// comment integration tests
UML_SERVER_SET_INTEGRATION_TEST(ElementOwnedComment, Comment, Class, &Element::getOwnedComments)
UML_SERVER_SET_INTEGRATION_TEST(CommentAnnotatedElement, Abstraction, Comment, &Comment::getAnnotatedElements)

// connector integration tests
UML_SERVER_SET_INTEGRATION_TEST(ConnectorConnectorEnds, ConnectorEnd, Connector, &Connector::getEnds)
UML_SERVER_SINGLETON_INTEGRATION_TEST(ConnectorType, Association, Connector, &Connector::getType, &Connector::setType)
UML_SERVER_SET_INTEGRATION_TEST(ConnectorContracts, OpaqueBehavior, Connector, &Connector::getContracts)
UML_SERVER_SINGLETON_INTEGRATION_TEST(ConnectorEndRole, Port, ConnectorEnd, &ConnectorEnd::getRole, &ConnectorEnd::setRole)

// dataType integration tests
UML_SERVER_SET_INTEGRATION_TEST(DataTypeOwnedAttributes, Property, DataType, &DataType::getOwnedAttributes)
UML_SERVER_SET_INTEGRATION_TEST(DataTypeOwnedOperations, Operation, PrimitiveType, &DataType::getOwnedOperations)
UML_SERVER_SINGLETON_INTEGRATION_TEST(PropertyDataType, DataType, Property, &Property::getDataType, &Property::setDataType)
UML_SERVER_SINGLETON_INTEGRATION_TEST(OperationDataType, DataType, Operation, &Operation::getDataType, &Operation::setDataType)

// dependency integration tests
UML_SERVER_SET_INTEGRATION_TEST(DependencySuppliers, Package, Dependency, &Dependency::getSuppliers)
UML_SERVER_SET_INTEGRATION_TEST(DependencyClients, Package, Dependency, &Dependency::getClients)
UML_SERVER_SET_INTEGRATION_TEST(NamedElementClientDependencies, Dependency, Package, &NamedElement::getClientDependencies)

// deployment integration tests
UML_SERVER_SET_INTEGRATION_TEST(DeploymentTargetDeployment, Deployment, InstanceSpecification, &DeploymentTarget::getDeployments)
UML_SERVER_SINGLETON_INTEGRATION_TEST(DeploymentLocation, Property, Deployment, &Deployment::getLocation, &Deployment::setLocation)
UML_SERVER_SET_INTEGRATION_TEST(DeploymentDeployedArtifacts, Artifact, Deployment, &Deployment::getDeployedArtifacts)

// element integration tests
UML_SERVER_SET_INTEGRATION_TEST(ElementAppliedStereotypes, InstanceSpecification, EnumerationLiteral, &Element::getAppliedStereotypes)

// enumeration integration tests
UML_SERVER_SET_INTEGRATION_TEST(EnumerationOwnedLiteral, EnumerationLiteral, Enumeration, &Enumeration::getOwnedLiterals)
UML_SERVER_SINGLETON_INTEGRATION_TEST(EnumerationLiteralEnumeration, Enumeration, EnumerationLiteral, &EnumerationLiteral::getEnumeration, &EnumerationLiteral::setEnumeration)

// extension integration tests
UML_SERVER_SINGLETON_INTEGRATION_TEST(ExtensionOwnedEnd, ExtensionEnd, Extension, &Extension::getOwnedEnd, &Extension::setOwnedEnd)

// generalization set integration tests
UML_SERVER_SINGLETON_INTEGRATION_TEST(GeneralizationSetPowerType, Class, GeneralizationSet, &GeneralizationSet::getPowerType, &GeneralizationSet::setPowerType)
UML_SERVER_SET_INTEGRATION_TEST(GeneralizationSetGeneralizations, Generalization, GeneralizationSet, &GeneralizationSet::getGeneralizations)
UML_SERVER_SET_INTEGRATION_TEST(GeneralizationGeneralizationSets, GeneralizationSet, Generalization, &Generalization::getGeneralizationSets)

// generalization integration tests
UML_SERVER_SINGLETON_INTEGRATION_TEST(GeneralizationSpecific, OpaqueBehavior, Generalization, &Generalization::getSpecific, &Generalization::setSpecific)
UML_SERVER_SINGLETON_INTEGRATION_TEST(GeneralizationGeneral, DataType, Generalization, &Generalization::getGeneral, &Generalization::setGeneral)

// instanceSpecification integration tests
UML_SERVER_SET_INTEGRATION_TEST(InstanceSpecificationSlots, Slot, InstanceSpecification, &InstanceSpecification::getSlots)
UML_SERVER_SINGLETON_INTEGRATION_TEST(InstanceSpecificationSpecification, LiteralUnlimitedNatural, InstanceSpecification, &InstanceSpecification::getSpecification, &InstanceSpecification::setSpecification)
UML_SERVER_SET_INTEGRATION_TEST(InstanceSpecificationClassifiers, Class, InstanceSpecification, &InstanceSpecification::getClassifiers)
UML_SERVER_SINGLETON_INTEGRATION_TEST(InstanceValueInstance, InstanceSpecification, InstanceValue, &InstanceValue::getInstance, &InstanceValue::setInstance)

// interface integration tests
UML_SERVER_SET_INTEGRATION_TEST(InterfaceOwnedAttribute, Property, Interface, &Interface::getOwnedAttributes)
UML_SERVER_SINGLETON_INTEGRATION_TEST(PropertyInterface, Interface, Property, &Property::getInterface, &Property::setInterface)
UML_SERVER_SET_INTEGRATION_TEST(InterfaceOwnedOperation, Operation, Interface, &Interface::getOwnedOperations)
UML_SERVER_SINGLETON_INTEGRATION_TEST(OperationInterface, Interface, Operation, &Operation::getInterface, &Operation::setInterface)
UML_SERVER_SET_INTEGRATION_TEST(InterfaceNestedClassifiers, Class, Interface, &Interface::getNestedClassifiers)
UML_SERVER_SINGLETON_INTEGRATION_TEST(InterfaceRealizationContract, Interface, InterfaceRealization, &InterfaceRealization::getContract, &InterfaceRealization::setContract)
UML_SERVER_SINGLETON_INTEGRATION_TEST(InterfaceRealizationImplementingClassifier, Class, InterfaceRealization, &InterfaceRealization::getImplementingClassifier, &InterfaceRealization::setImplementingClassifier)
UML_SERVER_SET_INTEGRATION_TEST(BehavioredClassifierInterfaceRealizations, InterfaceRealization, Class, &BehavioredClassifier::getInterfaceRealizations)

// manifestation integration tests
UML_SERVER_SINGLETON_INTEGRATION_TEST(ManifestationUtilizedElement, Package, Manifestation, &Manifestation::getUtilizedElement, &Manifestation::setUtilizedElement)

// multiplicityElement integration tests
UML_SERVER_SINGLETON_INTEGRATION_TEST(MultiplicityElementLowerValue, LiteralReal, Property, &MultiplicityElement::getLowerValue, &MultiplicityElement::setLowerValue)
UML_SERVER_SINGLETON_INTEGRATION_TEST(MultiplicityElementUpperValue, LiteralNull, Port, &MultiplicityElement::getUpperValue, &MultiplicityElement::setUpperValue)

// namespace integration tests
UML_SERVER_SET_INTEGRATION_TEST(NamespaceOwnedRules, Constraint, Package, &Namespace::getOwnedRules)
UML_SERVER_SINGLETON_INTEGRATION_TEST(ConstraintContext, Package, Constraint, &Constraint::getContext, &Constraint::setContext)
UML_SERVER_SET_INTEGRATION_TEST(NamespaceElementImports, ElementImport, Class, &Namespace::getElementImports)
UML_SERVER_SINGLETON_INTEGRATION_TEST(ElementImportImportingNamespace, Package, ElementImport, &ElementImport::getImportingNamespace, &ElementImport::setImportingNamespace)
UML_SERVER_SINGLETON_INTEGRATION_TEST(ElementImportImportedElement, DataType, ElementImport, &ElementImport::getImportedElement, &ElementImport::setImportedElement)
UML_SERVER_SET_INTEGRATION_TEST(NamespacePackageImports, PackageImport, Class, &Namespace::getPackageImports)
UML_SERVER_SINGLETON_INTEGRATION_TEST(PackageImportImportedPackage, Package, PackageImport, &PackageImport::getImportedPackage, &PackageImport::setImportedPackage)
UML_SERVER_SINGLETON_INTEGRATION_TEST(PackageImportImportingNamespace, OpaqueBehavior, PackageImport, &PackageImport::getImportingNamespace, &PackageImport::setImportingNamespace)

//operation integration tests
UML_SERVER_SET_INTEGRATION_TEST(OperationOwnedParameter, Parameter, Operation, &Operation::getOwnedParameters)
UML_SERVER_SINGLETON_INTEGRATION_TEST(ParameterOperation, Operation, Parameter, &Parameter::getOperation, &Parameter::setOperation)

//package integration tests
UML_SERVER_SET_INTEGRATION_TEST(PackagePackagedElements, DataType, Package, &Package::getPackagedElements)
UML_SERVER_SINGLETON_INTEGRATION_TEST(PackageableElementOwningPackage, Package, Package, &PackageableElement::getOwningPackage, &PackageableElement::setOwningPackage)
UML_SERVER_SET_INTEGRATION_TEST(PackagePackageMerges, PackageMerge, Package, &Package::getPackageMerge)
UML_SERVER_SINGLETON_INTEGRATION_TEST(PackageMergeReceivingPackage, Package, PackageMerge, &PackageMerge::getReceivingPackage, &PackageMerge::setReceivingPackage)
UML_SERVER_SINGLETON_INTEGRATION_TEST(PackageMergeMergedPackage, Package, PackageMerge, &PackageMerge::getMergedPackage, &PackageMerge::setMergedPackage)

// parameter integration tests
UML_SERVER_SINGLETON_INTEGRATION_TEST(ParameterDefaultValue, LiteralString, Parameter, &Parameter::getDefaultValue, &Parameter::setDefaultValue)
UML_SERVER_SET_INTEGRATION_TEST(ParameterParameterSets, ParameterSet, Parameter, &Parameter::getParameterSets)
UML_SERVER_SET_INTEGRATION_TEST(ParmeterSetParameters, Parameter, ParameterSet, &ParameterSet::getParameters)
UML_SERVER_SET_INTEGRATION_TEST(ParameterSetConditions, Constraint, ParameterSet, &ParameterSet::getConditions)
UML_SERVER_SET_INTEGRATION_TEST(BehavioralFeatureOwnedParameterSets, ParameterSet, Operation, &BehavioralFeature::getOwnedParameterSets)

// profileApplication integration tests
UML_SERVER_SET_INTEGRATION_TEST(PackageProfileApplication, ProfileApplication, Package, &Package::getProfileApplications)
UML_SERVER_SINGLETON_INTEGRATION_TEST(ProfileApplicationAppliedProfile, Profile, ProfileApplication, &ProfileApplication::getAppliedProfile, &ProfileApplication::setAppliedProfile)
UML_SERVER_SINGLETON_INTEGRATION_TEST(ProfileApplicationApplyingPackage, Package, ProfileApplication, &ProfileApplication::getApplyingPackage, &ProfileApplication::setApplyingPackage)

// property integration tests
UML_SERVER_SINGLETON_INTEGRATION_TEST(PropertyDefaultValue, Expression, Property, &Property::getDefaultValue, &Property::setDefaultValue)
UML_SERVER_SET_INTEGRATION_TEST(PropertyRedefinedProperties, Property, Property, &Property::getRedefinedProperties)

// signal integration tests
UML_SERVER_SET_INTEGRATION_TEST(SignalOwnedAttributes, Property, Signal, &Signal::getOwnedAttributes)
UML_SERVER_SINGLETON_INTEGRATION_TEST(ReceptionSignal, Signal, Reception, &Reception::getSignal, &Reception::setSignal)

// slot integration tests
UML_SERVER_SINGLETON_INTEGRATION_TEST(SlotDefiningFeature, Property, Slot, &Slot::getDefiningFeature, &Slot::setDefiningFeature)
UML_SERVER_SET_INTEGRATION_TEST(SlotValues, LiteralBool, Slot, &Slot::getValues)
UML_SERVER_SINGLETON_INTEGRATION_TEST(SlotOwningInstance, InstanceSpecification, Slot, &Slot::getOwningInstance, &Slot::setOwningInstance)

// templateableElement integration tests
UML_SERVER_SINGLETON_INTEGRATION_TEST(TemplateableElementOwnedTemplateSignature, TemplateSignature, Package, &TemplateableElement::getOwnedTemplateSignature, &TemplateableElement::setOwnedTemplateSignature)
UML_SERVER_SET_INTEGRATION_TEST(TemplateSignatureOwnedParameters, TemplateParameter, TemplateSignature, &TemplateSignature::getOwnedParameters)
UML_SERVER_SET_INTEGRATION_TEST(TemplateSignatureParameters, TemplateParameter, TemplateSignature, &TemplateSignature::getParameters)
UML_SERVER_SINGLETON_INTEGRATION_TEST(TemplateSignatureTemplate, Operation, TemplateSignature, &TemplateSignature::getTemplate, &TemplateSignature::setTemplate)
UML_SERVER_SINGLETON_INTEGRATION_TEST(TemplateParameterOwnedParameteredElement, Package, TemplateParameter, &TemplateParameter::getOwnedParameteredElement, &TemplateParameter::setOwnedParameteredElement)
UML_SERVER_SINGLETON_INTEGRATION_TEST(ParameterableElementOwningtemplateParameter, TemplateParameter, Package, &ParameterableElement::getOwningTemplateParameter, &ParameterableElement::setOwningTemplateParameter)
UML_SERVER_SINGLETON_INTEGRATION_TEST(ParameterableElementTemplateParameter, TemplateParameter, Package, &ParameterableElement::getTemplateParameter, &ParameterableElement::setTemplateParameter)
UML_SERVER_SINGLETON_INTEGRATION_TEST(TemplateParameterOwnedDefault, Package, TemplateParameter, &TemplateParameter::getOwnedDefault, &TemplateParameter::setOwnedDefault)
UML_SERVER_SINGLETON_INTEGRATION_TEST(TemplateParameterDefault, Package, TemplateParameter, &TemplateParameter::getDefault, &TemplateParameter::setDefault)
UML_SERVER_SINGLETON_INTEGRATION_TEST(TemplateParameterSubstitutionFormal, TemplateParameter, TemplateParameterSubstitution, &TemplateParameterSubstitution::getFormal, &TemplateParameterSubstitution::setFormal)
UML_SERVER_SINGLETON_INTEGRATION_TEST(TemplateParameterSubstitutionActual, Package, TemplateParameterSubstitution, &TemplateParameterSubstitution::getActual, &TemplateParameterSubstitution::setActual)
UML_SERVER_SINGLETON_INTEGRATION_TEST(TemplateParameterSubstitutionOwnedActual, Package, TemplateParameterSubstitution, &TemplateParameterSubstitution::getOwnedActual, &TemplateParameterSubstitution::setOwnedActual)
UML_SERVER_SINGLETON_INTEGRATION_TEST(TemplateParameterSubstitutionTemplateBinding, TemplateBinding, TemplateParameterSubstitution, &TemplateParameterSubstitution::getTemplateBinding, &TemplateParameterSubstitution::setTemplateBinding)
UML_SERVER_SET_INTEGRATION_TEST(TemplateBindingParameterSubstitution, TemplateParameterSubstitution, TemplateBinding, &TemplateBinding::getParameterSubstitutions)
UML_SERVER_SINGLETON_INTEGRATION_TEST(TemplateBindingBoundElement, Operation, TemplateBinding, &TemplateBinding::getBoundElement, &TemplateBinding::setBoundElement)
UML_SERVER_SET_INTEGRATION_TEST(TemplateableElementTemplateBindings, TemplateBinding, Package, &TemplateableElement::getTemplateBindings)
UML_SERVER_SINGLETON_INTEGRATION_TEST(TemplateBindingSignature, TemplateSignature, TemplateBinding, &TemplateBinding::getSignature, &TemplateBinding::setSignature)

// valueSpecification integration tests
UML_SERVER_SET_INTEGRATION_TEST(ExpressionOperands, Expression, Expression, &Expression::getOperands)
